/*
 * VGA text mode driver — superseded by the framebuffer driver.
 * Kept for fallback and for vga_entry/vga_entry_color helpers
 * still used by the terminal history buffer.
 * Functions that write to 0xB8000 or VGA hardware ports are stubbed out
 * or forwarded to the framebuffer driver.
 */

#include <drivers/framebuffer.h>
#include <drivers/vga.h>
#include <fonts/font8x16.h>
#include <ports.h>

uint8_t TERMINAL_COLOR;

static const uint32_t vga_to_rgb[16] = {
    0x00000000, 0x000000AA, 0x0000AA00, 0x0000AAAA,
    0x00AA0000, 0x00AA00AA, 0x00AA5500, 0x00AAAAAA,
    0x00555555, 0x005555FF, 0x0055FF55, 0x0055FFFF,
    0x00FF5555, 0x00FF55FF, 0x00FFFF55, 0x00FFFFFF,
};

extern uint32_t g_fb_width;
extern uint32_t g_fb_height;
extern uint32_t g_fb_pitch;
extern uint64_t g_fb_addr;

static void fb_putchar_raw(char c, uint32_t x, uint32_t y, uint32_t fg, uint32_t bg)
{
    unsigned char uc = (unsigned char)c;
    const unsigned char *glyph = &font8x16[uc][0];
    for (uint32_t row = 0; row < 16; row++) {
        unsigned char line = glyph[row];
        for (uint32_t col = 0; col < 8; col++) {
            uint32_t px = x + col;
            uint32_t py = y + row;
            if (px >= g_fb_width || py >= g_fb_height) {
                continue;
            }
            uint32_t color = (line & (0x80 >> col)) ? fg : bg;
            volatile uint32_t *pixel = (volatile uint32_t *)(g_fb_addr + (uint64_t)py * g_fb_pitch);
            pixel[px] = color;
        }
    }
}

/* Forward to framebuffer character drawing. */
void putentryat(char c, uint8_t color, size_t x, size_t y) {
    uint32_t fg = vga_to_rgb[color & 0xF];
    uint32_t bg = vga_to_rgb[(color >> 4) & 0xF];
    fb_putchar_raw(c, x * 8, y * 16, fg, bg);
}

uint8_t vga_entry_color(enum VGA_COLOR fg, enum VGA_COLOR bg) {
	return fg | bg << 4;
}

uint16_t vga_entry(unsigned char c, uint8_t color) {
	return (uint16_t) c | (uint16_t) color << 8;
}

/* Forward to framebuffer clear using the background color from the VGA byte. */
void vga_clear(uint8_t color) {
	uint32_t bg = vga_to_rgb[(color >> 4) & 0xF];
	fb_clear(bg);
}

/* Stubbed — hardware VGA cursor is no longer used. */
void move_tcursor(int x, int y) {
	(void)x;
	(void)y;
}

void set_termcolor(enum VGA_COLOR FG, enum VGA_COLOR BG) {
	TERMINAL_COLOR = vga_entry_color(FG, BG);
}
