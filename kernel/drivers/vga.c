#include <drivers/vga.h>
#include <ports.h>

uint8_t TERMINAL_COLOR;

void putentryat(char c, uint8_t color, size_t x, size_t y) {
	size_t index = y * VGA_TEXT_WIDTH + x;
	((uint16_t*)VGA_TEXT_ADDR)[index] = vga_entry(c, color);
}

uint8_t vga_entry_color(enum VGA_COLOR fg, enum VGA_COLOR bg) {
	return fg | bg << 4;
}

// I'm tired of commenting. - bonk enjoyer (dorito girl)

uint16_t vga_entry(unsigned char c, uint8_t color) {
	return (uint16_t) c | (uint16_t) color << 8;
}

void vga_clear(uint8_t color) {
	uint16_t off = 0;
	while (off < VGA_TEXT_WIDTH * VGA_TEXT_HEIGHT) {
		((uint16_t*)VGA_TEXT_ADDR)[off] = vga_entry(' ', color);
		off++;
	}
}

void move_tcursor(int x, int y) {
	uint16_t pos = y * VGA_TEXT_WIDTH + x;

	outb(0x3D4, 0xF);
	outb(0x3d5, (uint8_t) (pos & 0xFF));
	outb(0x3d4, 0xE);
	outb(0x3d5, (uint8_t) ((pos >> 8) & 0xFF));
}

void set_termcolor(enum VGA_COLOR FG, enum VGA_COLOR BG) {
	TERMINAL_COLOR = vga_entry_color(FG, BG);
}
