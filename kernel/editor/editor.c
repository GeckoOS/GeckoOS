//ember2819
#include <editor/editor.h>
#include <terminal/terminal.h>
#include <drivers/vga.h>
#include <drivers/keyboard.h>
#include <fs/fat32.h>
#include <fs/fs.h>
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#define ED_W  VGA_TEXT_WIDTH
#define ED_H  VGA_TEXT_HEIGHT
#define ED_ROWS (ED_H - 1)   // Last row reserved for status bar

// Scancodes
#define SC_BACKSPACE  0x0E
#define SC_ENTER      0x1C
#define SC_CTRL_MAKE  0x1D

static char    ed_buf[EDITOR_MAX_FILESIZE];
static int     ed_len;
static int     ed_cur;
static int     ed_view_row;
static char    ed_filename[32];
static bool    ed_dirty;
static uint8_t ed_color_text;
static uint8_t ed_color_status;

static int ed_line_start(int n) {
    int pos = 0, line = 0;
    while (line < n && pos < ed_len) {
        if (ed_buf[pos] == '\n') line++;
        pos++;
    }
    return pos;
}

static int ed_count_lines(void) {
    int lines = 1;
    for (int i = 0; i < ed_len; i++)
        if (ed_buf[i] == '\n') lines++;
    return lines;
}

static int ed_offset_to_line(int offset) {
    int line = 0;
    for (int i = 0; i < offset && i < ed_len; i++)
        if (ed_buf[i] == '\n') line++;
    return line;
}

static int ed_offset_to_col(int offset) {
    int col = 0, i = offset - 1;
    while (i >= 0 && ed_buf[i] != '\n') { col++; i--; }
    return col;
}

static int ed_line_len(int n) {
    int start = ed_line_start(n), len = 0;
    while (start + len < ed_len && ed_buf[start + len] != '\n') len++;
    return len;
}

static void ed_fill_row(uint8_t row, uint8_t color) {
    for (int c = 0; c < ED_W; c++)
        putentryat(' ', color, c, row);
}

static void ed_draw_status(void) {
    ed_fill_row(ED_H - 1, ed_color_status);
    int col = 0;
    const char *prefix = " GeckoEdit | ";
    for (; prefix[col]; col++)
        putentryat(prefix[col], ed_color_status, col, ED_H - 1);
    for (int i = 0; ed_filename[i] && col < ED_W - 20; i++, col++)
        putentryat(ed_filename[i], ed_color_status, col, ED_H - 1);
    if (ed_dirty) {
        const char *mod = " [modified]";
        for (int i = 0; mod[i] && col < ED_W - 18; i++, col++)
            putentryat(mod[i], ed_color_status, col, ED_H - 1);
    }
    const char *hints = "^S Save  ^Q Quit ";
    int hlen = 0;
    for (; hints[hlen]; hlen++);
    int hstart = ED_W - hlen;
    for (int i = 0; hints[i]; i++)
        putentryat(hints[i], ed_color_status, hstart + i, ED_H - 1);
}

static void ed_redraw(void) {
    int cur_line = ed_offset_to_line(ed_cur);
    if (cur_line < ed_view_row)
        ed_view_row = cur_line;
    if (cur_line >= ed_view_row + ED_ROWS)
        ed_view_row = cur_line - ED_ROWS + 1;

    for (int row = 0; row < ED_ROWS; row++) {
        int line = ed_view_row + row;
        ed_fill_row(row, ed_color_text);
        if (line >= ed_count_lines()) continue;
        int start = ed_line_start(line), col = 0;
        while (start < ed_len && ed_buf[start] != '\n' && col < ED_W) {
            putentryat(ed_buf[start], ed_color_text, col, row);
            col++; start++;
        }
    }

    ed_draw_status();

    int scr_row = cur_line - ed_view_row;
    int scr_col = ed_offset_to_col(ed_cur);
    if (scr_col >= ED_W) scr_col = ED_W - 1;
    move_tcursor(scr_col, scr_row);
}

static void ed_insert(char c) {
    if (ed_len >= EDITOR_MAX_FILESIZE - 1) return;
    for (int i = ed_len; i > ed_cur; i--)
        ed_buf[i] = ed_buf[i - 1];
    ed_buf[ed_cur] = c;
    ed_cur++;
    ed_len++;
    ed_buf[ed_len] = '\0';
    ed_dirty = true;
}

static void ed_backspace(void) {
    if (ed_cur == 0) return;
    ed_cur--;
    for (int i = ed_cur; i < ed_len - 1; i++)
        ed_buf[i] = ed_buf[i + 1];
    ed_len--;
    ed_buf[ed_len] = '\0';
    ed_dirty = true;
}

static void ed_move_left(void)  { if (ed_cur > 0)     ed_cur--; }
static void ed_move_right(void) { if (ed_cur < ed_len) ed_cur++; }

static void ed_move_up(void) {
    int line = ed_offset_to_line(ed_cur);
    if (line == 0) return;
    int col = ed_offset_to_col(ed_cur);
    int prev_len = ed_line_len(line - 1);
    if (col > prev_len) col = prev_len;
    ed_cur = ed_line_start(line - 1) + col;
}

static void ed_move_down(void) {
    int line  = ed_offset_to_line(ed_cur);
    int total = ed_count_lines();
    if (line >= total - 1) return;
    int col = ed_offset_to_col(ed_cur);
    int next_len = ed_line_len(line + 1);
    if (col > next_len) col = next_len;
    ed_cur = ed_line_start(line + 1) + col;
}
//save
static void ed_save(struct drive_fs_t *fs) {
    if (!fs) {
        ed_fill_row(ED_H - 1, ed_color_status);
        const char *msg = " [ERROR: filesystem not mounted]";
        for (int i = 0; msg[i] && i < ED_W; i++)
            putentryat(msg[i], ed_color_status, i, ED_H - 1);
        return;
    }
    int r = fat32_write_file(fs, ed_filename,
                             (const uint8_t *)ed_buf, (size_t)ed_len);
    if (r == 0) ed_dirty = false;
    ed_draw_status();
}

static void ed_load(struct drive_fs_t *fs) {
    ed_len = 0;
    ed_buf[0] = '\0';
    if (!fs) return;

    struct fs_entries_t entries = fs->get_entries((void *)fs);
    int found = -1;
    for (int i = 0; i < (int)entries.count; i++) {
        if (entries.entries[i].type != ENTRY_FILE) continue;
        const char *a = entries.entries[i].file.name;
        const char *b = ed_filename;
        int match = 1;
        while (*a && *b) {
            char ca = (*a >= 'a' && *a <= 'z') ? *a - 32 : *a;
            char cb = (*b >= 'a' && *b <= 'z') ? *b - 32 : *b;
            if (ca != cb) { match = 0; break; }
            a++; b++;
        }
        if (match && *a == '\0' && *b == '\0') { found = i; break; }
    }
    if (found < 0) return;

    int offset = 0;
    uint8_t tmp[128];
    int bytes;
    while (ed_len < EDITOR_MAX_FILESIZE - 1) {
        bytes = entries.entries[found].file.read(
            (void *)&entries.entries[found].file, offset, 128, tmp);
        if (bytes <= 0) break;
        for (int k = 0; k < bytes && ed_len < EDITOR_MAX_FILESIZE - 1; k++)
            ed_buf[ed_len++] = (char)tmp[k];
        offset += bytes;
    }
    ed_buf[ed_len] = '\0';
}

static void ed_strcpy(char *dst, const char *src, int maxlen) {
    int i = 0;
    while (src[i] && i < maxlen - 1) { dst[i] = src[i]; i++; }
    dst[i] = '\0';
}

void editor_run(struct drive_fs_t *fs, const char *filename, uint8_t color) {
    ed_strcpy(ed_filename, filename, 32);
    ed_cur      = 0;
    ed_view_row = 0;
    ed_dirty    = false;

    ed_color_text   = vga_entry_color(VGA_COLOR_CYAN,  VGA_COLOR_BLACK);
    ed_color_status = vga_entry_color(VGA_COLOR_BLACK, VGA_COLOR_CYAN);

    vga_clear(ed_color_text);
    ed_load(fs);
    ed_redraw();

    bool ctrl_held = false;

    while (true) {
        scancode_t sc = ps2_kb_wfi();
        bool is_break  = (sc & 0x80) != 0;
        scancode_t base = sc & 0x7F;

        // Track Ctrl key
        if (base == SC_CTRL_MAKE) { ctrl_held = !is_break; continue; }

        // Ignore key-release events
        if (is_break) { continue; }

        // Arrow keys — all four delivered as KEY_* by ps2_kb_wfi
        if (sc == KEY_UP)    { ed_move_up();    ed_redraw(); continue; }
        if (sc == KEY_DOWN)  { ed_move_down();  ed_redraw(); continue; }
        if (sc == KEY_LEFT)  { ed_move_left();  ed_redraw(); continue; }
        if (sc == KEY_RIGHT) { ed_move_right(); ed_redraw(); continue; }

        // Ctrl+S (scancode 0x1F = 's')
        if (ctrl_held && sc == 0x1F) { ed_save(fs); ed_redraw(); continue; }

        // Ctrl+Q (scancode 0x10 = 'q')
        if (ctrl_held && sc == 0x10) {
            if (ed_dirty) {
                ed_fill_row(ED_H - 1, ed_color_status);
                const char *msg = " Unsaved! ^Q again to discard, any key to cancel.";
                for (int i = 0; msg[i] && i < ED_W; i++)
                    putentryat(msg[i], ed_color_status, i, ED_H - 1);
                scancode_t c2;
                do { c2 = ps2_kb_wfi(); } while (c2 & 0x80);
                if (c2 == 0x10 && ctrl_held) break;
                ed_redraw();
                continue;
            }
            break;
        }

        // Backspace
        if (sc == SC_BACKSPACE) { ed_backspace(); ed_redraw(); continue; }

        // Enter
        if (sc == SC_ENTER) { ed_insert('\n'); ed_redraw(); continue; }

        // Printable character
        unsigned char ascii = scancode_to_ascii(sc);
        if (ascii >= 0x20 && ascii < 0x80) { ed_insert((char)ascii); ed_redraw(); }
    }

    // Restore terminal
    terminal_clear(color);
}