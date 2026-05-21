#include <boot/multiboot2.h>
#include <drivers/framebuffer.h>
#include <stdint.h>

#define VGA_COLOR_BLACK     0
#define VGA_COLOR_LIGHT_RED 12

#define MULTIBOOT1_BOOTLOADER_MAGIC 0x2BADB002

uint64_t g_mbi_addr         = 0;
uint64_t g_multiboot2_magic = 0;

uint64_t g_fb_addr;
uint32_t g_fb_width;
uint32_t g_fb_height;
uint32_t g_fb_pitch;
uint8_t  g_fb_bpp;

extern void _entry(void);

void multiboot2_main(uint64_t magic, uint64_t mbi_addr)
{
    g_multiboot2_magic = magic;
    g_mbi_addr         = mbi_addr;

    int valid = (magic == MULTIBOOT2_BOOTLOADER_MAGIC) ||
                (magic == MULTIBOOT1_BOOTLOADER_MAGIC);

    if (!valid) {
        volatile uint16_t *vga = (volatile uint16_t *)0xB8000;
        const char msg[] = "BAD MULTIBOOT MAGIC";
        uint8_t attr = (VGA_COLOR_BLACK << 4) | VGA_COLOR_LIGHT_RED;
        for (int i = 0; msg[i]; i++) {
            vga[i] = (uint16_t)((attr << 8) | (uint8_t)msg[i]);
        }
        for (;;) { __asm__ volatile ("cli; hlt"); }
    }

    if (magic == MULTIBOOT2_BOOTLOADER_MAGIC) {
        multiboot2_info_header_t *mbi = (multiboot2_info_header_t *)(uintptr_t)mbi_addr;
        multiboot2_tag_t *tag = (multiboot2_tag_t *)((uintptr_t)mbi + sizeof(multiboot2_info_header_t));

        for (;;) {
            if (tag->type == MULTIBOOT2_TAG_TYPE_END) {
                break;
            }
            if (tag->type == MULTIBOOT2_TAG_TYPE_FRAMEBUFFER) {
                struct multiboot2_tag_framebuffer *fb = (struct multiboot2_tag_framebuffer *)tag;
                g_fb_addr   = fb->framebuffer_addr;
                g_fb_width  = fb->framebuffer_width;
                g_fb_height = fb->framebuffer_height;
                g_fb_pitch  = fb->framebuffer_pitch;
                g_fb_bpp    = fb->framebuffer_bpp;

                fb_init(g_fb_addr, g_fb_width, g_fb_height, g_fb_pitch, g_fb_bpp);
            }
            tag = MULTIBOOT2_TAG_NEXT(tag);
        }
    }

    _entry();

    for (;;) { __asm__ volatile ("cli; hlt"); }
}
