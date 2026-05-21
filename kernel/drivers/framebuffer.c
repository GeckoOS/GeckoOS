#include <drivers/framebuffer.h>
#include <fonts/font8x16.h>
#include <mem.h>
#include <stdint.h>

static uint64_t fb_base_addr;
static uint32_t fb_width;
static uint32_t fb_height;
static uint32_t fb_pitch;
static uint8_t  fb_bpp;

void fb_init(uint64_t addr, uint32_t width, uint32_t height, uint32_t pitch, uint8_t bpp)
{
    fb_base_addr = addr;
    fb_width     = width;
    fb_height    = height;
    fb_pitch     = pitch;
    fb_bpp       = bpp;
}

void fb_draw_pixel(uint32_t x, uint32_t y, uint32_t color)
{
    if (x >= fb_width || y >= fb_height) {
        return;
    }
    volatile uint32_t *pixel = (volatile uint32_t *)(fb_base_addr + (uint64_t)y * fb_pitch);
    pixel[x] = color;
}

void fb_draw_char(uint32_t x, uint32_t y, char c, uint32_t fg, uint32_t bg)
{
    unsigned char uc = (unsigned char)c;
    const unsigned char *glyph = &font8x16[uc][0];

    for (uint32_t row = 0; row < 16; row++) {
        unsigned char line = glyph[row];
        for (uint32_t col = 0; col < 8; col++) {
            uint32_t px = x + col;
            uint32_t py = y + row;
            if (px >= fb_width || py >= fb_height) {
                continue;
            }
            uint32_t color = (line & (0x80 >> col)) ? fg : bg;
            volatile uint32_t *pixel = (volatile uint32_t *)(fb_base_addr + (uint64_t)py * fb_pitch);
            pixel[px] = color;
        }
    }
}

void fb_scroll(void)
{
    uint64_t bytes_per_row = fb_pitch;
    uint64_t scroll_bytes  = (uint64_t)16 * bytes_per_row;
    uint64_t total_bytes   = (uint64_t)fb_height * bytes_per_row;

    memmove((void *)fb_base_addr,
            (void *)(fb_base_addr + scroll_bytes),
            total_bytes - scroll_bytes);

    uint64_t clear_start = total_bytes - scroll_bytes;
    memset((void *)(fb_base_addr + clear_start), 0, scroll_bytes);
}

void fb_clear(uint32_t color)
{
    uint64_t total_pixels = (uint64_t)fb_height * (fb_pitch / 4);
    volatile uint32_t *buf = (volatile uint32_t *)fb_base_addr;
    for (uint64_t i = 0; i < total_pixels; i++) {
        buf[i] = color;
    }
}
