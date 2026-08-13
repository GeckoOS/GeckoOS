# Upload server

Host-side server that gets files into the VM. The guest downloads

## Run

```sh
python app.py
```

The web UI is at `http://127.0.0.1:5000`

Files are put into `server/files/` and are served to the VM through UDP port 8080. (It may change to TCP once I implement that)

The VM sees the host (QEMU slirp) at `10.0.2.2`, so from the guest send requests to `10.0.2.2:8080`.

## Pulling a file from the guest

Only one outstanding request is supported at a time, so chunk requests must be sequential.
