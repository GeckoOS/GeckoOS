#include "terminal/printf.h"
#include <drivers/framebuffer.h>
#include <drivers/keyboard.h>
#include <drivers/mouse.h>
#include <gk/gk.h>
#include <mem.h>
#include <stddef.h>
#include <stdint.h>
#include <terminal/terminal.h>

extern uint64_t g_fb_addr;
extern uint32_t g_fb_width;
extern uint32_t g_fb_height;
extern uint32_t g_fb_pitch;
extern uint8_t  g_fb_bpp;

#define FONT_WIDTH  8
#define FONT_HEIGHT 16

static uint32_t cursor_x = 0;

static uint32_t fb_cols;
static uint32_t fb_rows;

static void fb_update_dims(void)
{
    fb_cols = g_fb_width / FONT_WIDTH;
    fb_rows = g_fb_height / FONT_HEIGHT;
}

static const uint32_t vga_to_rgb[16] = {
    0x00000000,
    0x000000AA,
    0x0000AA00,
    0x0000AAAA,
    0x00AA0000,
    0x00AA00AA,
    0x00AA5500,
    0x00AAAAAA,
    0x00555555,
    0x005555FF,
    0x0055FF55,
    0x0055FFFF,
    0x00FF5555,
    0x00FF55FF,
    0x00FFFF55,
    0x00FFFFFF,
};

static uint32_t vga_fg_to_rgb(uint8_t color)
{
    return vga_to_rgb[color & 0xF];
}

static uint32_t vga_bg_to_rgb(uint8_t color)
{
    return vga_to_rgb[(color >> 4) & 0xF];
}

static void fb_draw_cursor_block(uint32_t cx, uint32_t cy, uint32_t bg)
{
    for (uint32_t row = 0; row < FONT_HEIGHT; row++) {
        for (uint32_t col = 0; col < FONT_WIDTH; col++) {
            uint32_t px = cx * FONT_WIDTH + col;
            uint32_t py = cy * FONT_HEIGHT + row;
            if (px >= g_fb_width || py >= g_fb_height) {
                continue;
            }
            uint32_t color = (row >= FONT_HEIGHT - 2) ? 0x00FFFFFF : bg;
            volatile uint32_t *pixel = (volatile uint32_t *)(g_fb_addr + (uint64_t)py * g_fb_pitch);
            pixel[px] = color;
        }
    }
}

uint16_t terminal_column = 0;
uint16_t terminal_row    = 0;
static int history_count = 0;
static int history_head  = 0;

static bool follow_output = true;

void putchar(char c, uint8_t color)
{
    if (fb_cols == 0) {
        fb_update_dims();
    }

    uint32_t fg = vga_fg_to_rgb(color);
    uint32_t bg = vga_bg_to_rgb(color);

    if (c == 0) {
        return;
    }

    if (c == '\n') {
        /* Blank the rest of this history row so render_viewport is clean */
        for (uint32_t k = cursor_x; k < fb_cols && k < VGA_TEXT_WIDTH; k++) {
            console_history[total_lines][k] = vga_entry(' ', color);
        }
        cursor_x = 0;
        total_lines++;
        if (total_lines >= SCROLLBACK_LINES)
            total_lines = SCROLLBACK_LINES - 1;
    } else if (c == '\b') {
        if (cursor_x > 0) {
            cursor_x--;
        } else if (total_lines > 0) {
            total_lines--;
            cursor_x = (fb_cols < VGA_TEXT_WIDTH ? fb_cols : VGA_TEXT_WIDTH) - 1;
        }
        console_history[total_lines][cursor_x] = vga_entry(' ', color);
    } else if (c == '\t') {
        for (int j = 0; j < 4; j++) {
            putchar(' ', color);
        }
        return;
    } else {
        if (cursor_x < VGA_TEXT_WIDTH) {
            console_history[total_lines][cursor_x] = vga_entry(c, color);
        }
        cursor_x++;
    }

    /* Word-wrap */
    if (cursor_x >= fb_cols) {
        cursor_x = 0;
        total_lines++;
        if (total_lines >= SCROLLBACK_LINES)
            total_lines = SCROLLBACK_LINES - 1;
    }

    /* Compute the screen row this character belongs to */
    int screen_row = (int)total_lines - viewport_top;

    /* Update viewport to follow output; full redraw only when it scrolls */
    if (follow_output) {
        int new_top = (int)total_lines - (int)fb_rows + 1;
        if (new_top < 0) new_top = 0;
        if (new_top != viewport_top) {
            viewport_top = new_top;
            screen_row   = (int)total_lines - viewport_top;
            render_viewport();
            terminal_row    = (uint16_t)(total_lines - viewport_top);
            terminal_column = cursor_x;
            return;
        }
    }

    /* Fast path: draw only the character that just changed */
    if (screen_row >= 0 && screen_row < (int)fb_rows) {
        if (c == '\b') {
            fb_draw_char(cursor_x * FONT_WIDTH,
                         (uint32_t)screen_row * FONT_HEIGHT,
                         ' ', fg, bg);
        } else if (c != '\n') {
            uint32_t draw_col = cursor_x > 0 ? cursor_x - 1 : 0;
            fb_draw_char(draw_col * FONT_WIDTH,
                         (uint32_t)screen_row * FONT_HEIGHT,
                         c, fg, bg);
        }
    }

    terminal_row    = (uint16_t)(total_lines - viewport_top);
    terminal_column = cursor_x;
}

void write(char *data, size_t size, uint8_t COLOR)
{
    for (int i = 0; i < size; i++) {
        putchar(data[i], COLOR);
    }
}

void printc(char *data, uint8_t COLOR)
{
    for (size_t i = 0; data[i]; i++) {
        putchar(data[i], COLOR);
    }
}

void kprintf(int severity, char *data, ...)
{
    for (size_t i = 0; data[i]; i++) {
        putchar(data[i], VGA_COLOR_WHITE);
    }
}

void print(char *data)
{
    for (size_t i = 0; data[i]; i++) {
        putchar(data[i], VGA_COLOR_WHITE);
    }
}

void print_int(int n)
{
    char buff[32];
    int_to_str(n, buff);
    print(buff);
}

void render_viewport()
{
    for (size_t y = 0; y < fb_rows && (size_t)(viewport_top + y) < SCROLLBACK_LINES; y++) {
        for (size_t x = 0; x < fb_cols; x++) {
            uint16_t entry = console_history[viewport_top + y][x];
            char c = (char)(entry & 0xFF);
            uint8_t color = (uint8_t)(entry >> 8);
            uint32_t fg = vga_fg_to_rgb(color);
            uint32_t bg = vga_bg_to_rgb(color);
            fb_draw_char(x * FONT_WIDTH, y * FONT_HEIGHT, c, fg, bg);
        }
    }
}

void scroll_up()
{
    if (viewport_top >= total_lines - (int)fb_rows + 1) {
        follow_output = false;
    }
    if (viewport_top == 0)
        return;
    if (viewport_top >= 0)
        viewport_top--;
}

void scroll_down()
{
    if (viewport_top >= total_lines - (int)fb_rows + 1) {
        follow_output = true;
    }
    if (viewport_top + (int)fb_rows <= total_lines)
        viewport_top++;
}

void terminal_clear(uint8_t color)
{
    uint32_t bg = vga_bg_to_rgb(color);
    fb_clear(bg);

    uint16_t blank = vga_entry(' ', color);
    for (size_t y = 0; y < SCROLLBACK_LINES; y++) {
        for (size_t x = 0; x < VGA_TEXT_WIDTH; x++) {
            console_history[y][x] = blank;
        }
    }
    total_lines     = 0;
    cursor_x        = 0;
    terminal_column = 0;
    terminal_row    = 0;
    viewport_top    = 0;
}

static void history_push(unsigned char *buf)
{
    if (buf[0] == '\0')
        return;
    size_t i;
    for (i = 0; buf[i] && i < 511; i++) {
        history_entries[history_head][i] = buf[i];
    }
    history_entries[history_head][i] = '\0';
    history_head                     = (history_head + 1) % HISTORY_SIZE;
    if (history_count < HISTORY_SIZE)
        history_count++;
}

void input(unsigned char *buff, size_t buffer_size, uint8_t color)
{
    size_t buff_count  = 0;
    size_t start_x     = cursor_x;
    size_t start_line  = total_lines;  /* absolute history line at prompt */

    int browse_idx = 0;
    unsigned char saved_input[512];

    while (true) {
        scancode_t sc = ps2_kb_wfi();

        if (sc & 0x80)
            continue;
        if (sc == 0)
            continue;

        if (sc == KEY_UP || sc == KEY_DOWN) {
            if (sc == KEY_UP && browse_idx == 0) {
                size_t k;
                for (k = 0; k < buff_count; k++) {
                    saved_input[k] = buff[k];
                }
                saved_input[k] = '\0';
            }

            if (sc == KEY_UP && browse_idx < history_count)
                browse_idx++;
            if (sc == KEY_DOWN && browse_idx > 0)
                browse_idx--;

            unsigned char *src;
            if (browse_idx == 0) {
                src = saved_input;
            } else {
                int slot =
                    (history_head - browse_idx + HISTORY_SIZE) % HISTORY_SIZE;
                src = history_entries[slot];
            }

            /* Erase current input from history buffer, reset cursor */
            for (size_t k = 0; k < buff_count; k++) {
                size_t abs_col   = start_x + k;
                size_t hist_line = start_line + abs_col / fb_cols;
                size_t hist_col  = abs_col % fb_cols;
                if (hist_col < VGA_TEXT_WIDTH)
                    console_history[hist_line][hist_col] = vga_entry(' ', color);
            }
            cursor_x    = start_x;
            total_lines = start_line;
            render_viewport();

            size_t k;
            for (k = 0; src[k] && k < buffer_size - 1; k++) {
                buff[k] = src[k];
                putchar(src[k], color);
            }

            buff_count       = k;
            buff[buff_count] = '\0';
            continue;
        }

        unsigned char ascii = scancode_to_ascii(sc);

        if (ascii == '\n')
            break;

        if (ascii == '\b') {
            if (buff_count > 0) {
                putchar('\b', color);
                buff_count--;
                buff[buff_count] = 0;
            }
            continue;
        }

        if (buff_count < buffer_size - 1 && ascii >= 0x20) {
            buff[buff_count] = ascii;
            putchar(ascii, color);
            buff_count++;
        }
    }

    buff[buff_count] = '\0';
    history_push(buff);
}

void print_hex(uint32_t n)
{
    int32_t tmp;

    print("0x");

    char noZeroes = 1;

    int i;
    for (i = 28; i > 0; i -= 4) {
        tmp = (n >> i) & 0xF;
        if (tmp == 0 && noZeroes != 0) {
            continue;
        }

        if (tmp >= 0xA) {
            noZeroes = 0;
            putchar(tmp - 0xA + 'a', VGA_COLOR_WHITE);
        } else {
            noZeroes = 0;
            putchar(tmp + '0', VGA_COLOR_WHITE);
        }
    }

    tmp = n & 0xF;
    if (tmp >= 0xA) {
        putchar(tmp - 0xA + 'a', VGA_COLOR_WHITE);
    } else {
        putchar(tmp + '0', VGA_COLOR_WHITE);
    }
}

void terminal_init()
{
    fb_cols = g_fb_width / FONT_WIDTH;
    fb_rows = g_fb_height / FONT_HEIGHT;
    register_mouse_callback(get_mouse_event);
}

void get_mouse_event(mouse_event_data_t md)
{
    int scroll = md.scroll == 0 ? 0 : md.scroll - 8;
    if (scroll) {
        if (scroll < 0) {
            scroll_down();
        }
        if (scroll > 0) {
            scroll_up();
        }
    }
    render_viewport();
    draw_cursor(md.x, md.y, 1);
}

void draw_cursor(int x, int y, int visible)
{
    if (visible) {
        if (cursor.is_drawn) {
            uint32_t old_cx = cursor.x;
            uint32_t old_cy = cursor.y;
            uint16_t old_entry = console_history[viewport_top + old_cy][old_cx];
            char old_c = (char)(old_entry & 0xFF);
            uint8_t old_color = (uint8_t)(old_entry >> 8);
            uint32_t fg = vga_fg_to_rgb(old_color);
            uint32_t bg = vga_bg_to_rgb(old_color);
            fb_draw_char(old_cx * FONT_WIDTH, old_cy * FONT_HEIGHT, old_c, fg, bg);
            cursor.is_drawn = 0;
        }

        uint32_t cx = x;
        uint32_t cy = y;
        uint16_t entry = console_history[viewport_top + cy][cx];
        uint8_t old_color = (uint8_t)(entry >> 8);
        uint32_t bg = vga_bg_to_rgb(old_color);

        fb_draw_cursor_block(cx, cy, bg);

        cursor.x = cx;
        cursor.y = cy;
        cursor.is_drawn = 1;
    } else {
        if (cursor.is_drawn) {
            uint32_t cx = cursor.x;
            uint32_t cy = cursor.y;
            uint16_t entry = console_history[viewport_top + cy][cx];
            char c = (char)(entry & 0xFF);
            uint8_t color = (uint8_t)(entry >> 8);
            uint32_t fg = vga_fg_to_rgb(color);
            uint32_t bg = vga_bg_to_rgb(color);
            fb_draw_char(cx * FONT_WIDTH, cy * FONT_HEIGHT, c, fg, bg);
            cursor.is_drawn = 0;
        }
    }
}

static int console_history_tail = 0;

void add_to_history(void)
{
    if (console_history_tail >= SCROLLBACK_LINES) {
        console_history_tail = 0;
    }

    for (size_t x = 0; x < fb_cols && x < VGA_TEXT_WIDTH; x++) {
        uint32_t px = x * FONT_WIDTH;
        uint32_t py = 0;
        unsigned char glyph_line = 0;

        uint16_t entry = 0;
        for (uint32_t col = 0; col < FONT_WIDTH; col++) {
            if (px + col >= g_fb_width || py >= g_fb_height) {
                continue;
            }
            volatile uint32_t *pixel = (volatile uint32_t *)(g_fb_addr + (uint64_t)py * g_fb_pitch);
            if (pixel[px + col] != 0) {
                glyph_line |= (0x80 >> col);
            }
        }
        (void)glyph_line;
        console_history[console_history_tail][x] = entry;
    }

    console_history_tail++;
}