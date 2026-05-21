#ifndef FRAMEBUFFER_H
#define FRAMEBUFFER_H

#include <stdint.h>

void fb_init(uint64_t addr, uint32_t width, uint32_t height, uint32_t pitch, uint8_t bpp);
void fb_draw_pixel(uint32_t x, uint32_t y, uint32_t color);
void fb_draw_char(uint32_t x, uint32_t y, char c, uint32_t fg, uint32_t bg);
void fb_scroll(void);
void fb_clear(uint32_t color);

#endif
