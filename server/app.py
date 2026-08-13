import os
import socket
import threading
import time

from flask import (
    Flask,
    flash,
    redirect,
    render_template,
    request,
    send_from_directory,
    url_for,
)

FILES_DIR = os.path.join(os.path.dirname(os.path.abspath(__file__)), "files")
UDP_HOST = os.environ.get("GECKO_UDP_HOST", "0.0.0.0")
UDP_PORT = int(os.environ.get("GECKO_UDP_PORT", "8080"))

# The guest's UDP reply buffer holds 1472 bytes (UDP_REPLY_PAYLOAD_MAX),
# so chunk + header must stay well under that.
CHUNK_SIZE = 1430
MAX_REPLY = 1472

# UDP is connectionless, so there's no handshake to check. "Connected" is
# inferred as: we heard *any* datagram within this many seconds.
STATUS_TIMEOUT_SECONDS = 10

# per-file pull state, keyed by file name (only files served via GET)
PROGRESS_STALE_SECONDS = 60

os.makedirs(FILES_DIR, exist_ok=True)

app = Flask(__name__)
app.secret_key = "gecko-upload"

_last_seen_lock = threading.Lock()
_last_seen_ts = 0.0

_progress_lock = threading.Lock()
_progress = {}


def safe_path(name):
    name = os.path.basename(name.replace("\\", "/"))
    if not name or name in (".", ".."):
        return None
    return os.path.join(FILES_DIR, name)


def udp_reply(data):
    """Answers one plain-text request from the guest.

    Requests:
        LIST            -> "OK\n<name> <size>\n..."      (sorted, truncated)
        SIZE <name>     -> "OK <size>" | "NOENT"
        GET <name> <off>-> "OK <off> <total> <len>\n<data>" | "NOENT" | "EARG"

    GET is offset-based and idempotent, so retransmits are harmless and the
    guest can pull a file in sequential chunks.
    """
    try:
        parts = data.decode("latin-1").strip().split(" ")
    except Exception:
        return b"EARG"
    cmd = parts[0].upper()

    if cmd == "LIST":
        lines = ["OK"]
        for f in sorted(os.listdir(FILES_DIR)):
            p = os.path.join(FILES_DIR, f)
            if os.path.isfile(p):
                lines.append("%s %d" % (f, os.path.getsize(p)))
        return "\n".join(lines).encode("latin-1")[:MAX_REPLY]

    if cmd == "SIZE" and len(parts) == 2:
        p = safe_path(parts[1])
        if not p or not os.path.isfile(p):
            return b"NOENT"
        return ("OK %d" % os.path.getsize(p)).encode("latin-1")

    if cmd == "GET" and len(parts) == 3:
        p = safe_path(parts[1])
        if not p or not os.path.isfile(p):
            return b"NOENT"
        try:
            off = int(parts[2])
            if off < 0:
                off = 0
        except ValueError:
            return b"EARG"

        size = os.path.getsize(p)
        if off >= size:
            return ("OK %d %d 0" % (off, size)).encode("latin-1")

        with open(p, "rb") as fh:
            fh.seek(off)
            chunk = fh.read(CHUNK_SIZE)
        with _progress_lock:
            _progress[parts[1]] = {
                "size": size,
                "served": min(off + len(chunk), size),
                "t": time.time(),
            }
        return ("OK %d %d %d\n" % (off, size, len(chunk))).encode("latin-1") + chunk

    return b"EBADCMD"


def udp_loop():
    global _last_seen_ts
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.bind((UDP_HOST, UDP_PORT))
    print("UDP serving on %s:%d" % (UDP_HOST, UDP_PORT), flush=True)
    while True:
        data, addr = sock.recvfrom(65535)
        with _last_seen_lock:
            _last_seen_ts = time.time()
        try:
            out = udp_reply(data)
        except Exception:
            out = b"EERR"
        sock.sendto(out, addr)


def fmt_size(n):
    units = ("B", "KiB", "MiB", "GiB")
    i = 0
    while n >= 1024 and i < len(units) - 1:
        n /= 1024.0
        i += 1
    return ("%d %s" if i == 0 else "%.1f %s") % (n, units[i])


@app.route("/")
def index():
    files = []
    for f in sorted(os.listdir(FILES_DIR)):
        p = os.path.join(FILES_DIR, f)
        if os.path.isfile(p):
            files.append((f, fmt_size(os.path.getsize(p))))
    return render_template("index.html", files=files)


@app.route("/status")
def status():
    with _last_seen_lock:
        last_seen = _last_seen_ts
    connected = last_seen > 0 and (time.time() - last_seen) < STATUS_TIMEOUT_SECONDS
    return {"connected": connected, "last_seen": last_seen or None}


@app.route("/progress")
def progress():
    now = time.time()
    items = []
    with _progress_lock:
        for name, st in _progress.items():
            if now - st["t"] > PROGRESS_STALE_SECONDS:
                continue
            items.append({
                "name": name,
                "size": st["size"],
                "served": st["served"],
                "age": now - st["t"],
            })
    items.sort(key=lambda x: x["age"])
    return {"files": items}


@app.route("/upload", methods=["POST"])
def upload():
    f = request.files.get("file")
    if not f or not f.filename:
        flash("No file selected")
        return redirect(url_for("index"))
    name = os.path.basename(f.filename)
    f.save(os.path.join(FILES_DIR, name))
    flash("Uploaded %s" % name)
    return redirect(url_for("index"))


@app.route("/files/<path:name>")
def download(name):
    return send_from_directory(FILES_DIR, name)


if __name__ == "__main__":
    threading.Thread(target=udp_loop, daemon=True).start()
    print("Web UI: http://127.0.0.1:5000  (upload files for the VM here)")
    app.run(host="127.0.0.1", port=5000, use_reloader=False)