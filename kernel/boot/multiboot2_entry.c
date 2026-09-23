#include "drivers/vga.h"
#include "terminal/terminal.h"
#include <boot/multiboot2.h>
#include <drivers/framebuffer.h>
#include <stdint.h>
#include <terminal/printf.h>

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

uint32_t memsize_grub; // mem_lower * mem_upper
struct multiboot2_tag_acpi* acpi_info_grub = NULL;
uint64_t max_addr_used     = 0; // To map physical memory
struct multiboot2_mmap_entry max_mem_used; // For the heap

#ifdef DEBUG
    struct multiboot2_tag_bootloader_name* bootloader_info;
#endif

extern void _entry(uint64_t);

void multiboot2_main(uint64_t magic, uint64_t mbi_addr)
{
    g_multiboot2_magic = magic;
    g_mbi_addr         = mbi_addr;

    int valid = (magic == MULTIBOOT2_BOOTLOADER_MAGIC); // ||
            //    (magic == MULTIBOOT1_BOOTLOADER_MAGIC); // Why whould you support multiboot1 if the stuff that works with multiboot2 wont work, this is useless
            // Maybe someone will add support to multiboot1, but for now this will be removed

    if (!valid) { // Not working (Dosen't print anything with the new way to print things)
/*         volatile uint16_t *vga = (volatile uint16_t *)0xB8000;
        const char msg[] = "BAD MULTIBOOT MAGIC";
        uint8_t attr = (VGA_COLOR_BLACK << 4) | VGA_COLOR_LIGHT_RED;
        for (int i = 0; msg[i]; i++) {
            vga[i] = (uint16_t)((attr << 8) | (uint8_t)msg[i]);
        } */
        // asm ("mov %%rax, %0" : "r"(0xBAD) : : "%rax");
        for (;;) { __asm__ volatile ("cli; hlt"); }
    }

    if (magic == MULTIBOOT2_BOOTLOADER_MAGIC) {
        multiboot2_info_header_t *mbi = (multiboot2_info_header_t *)(uintptr_t)mbi_addr;
        multiboot2_tag_t *tag = (multiboot2_tag_t *)((uintptr_t)mbi + sizeof(multiboot2_info_header_t));

        for (;;) {
            switch (tag->type) {
                case MULTIBOOT2_TAG_TYPE_FRAMEBUFFER:
                    struct multiboot2_tag_framebuffer *fb = (struct multiboot2_tag_framebuffer *)tag;
                    g_fb_addr   = fb->framebuffer_addr;
                    g_fb_width  = fb->framebuffer_width;
                    g_fb_height = fb->framebuffer_height;
                    g_fb_pitch  = fb->framebuffer_pitch;
                    g_fb_bpp    = fb->framebuffer_bpp;

                    fb_init(g_fb_addr, g_fb_width, g_fb_height, g_fb_pitch, g_fb_bpp);
                    break;
                case MULTIBOOT2_TAG_TYPE_BASIC_MEMINFO:
                    memsize_grub = ((struct multiboot2_tag_basic_meminfo*)tag)->mem_lower * ((struct multiboot2_tag_basic_meminfo*)tag)->mem_upper;
                    break;
                case MULTIBOOT2_TAG_TYPE_ACPI_NEW:
                case MULTIBOOT2_TAG_TYPE_ACPI_OLD:
                    acpi_info_grub = ((struct multiboot2_tag_acpi*)tag);
                    break;
                #ifdef DEBUG
                    case MULTIBOOT2_TAG_TYPE_BOOT_LOADER_NAME:
                        bootloader_info = ((struct multiboot2_tag_bootloader_name*)tag);
                        break;
                #endif
                case MULTIBOOT2_TAG_TYPE_MMAP:
                    struct multiboot2_tag_mmap *mmap_tag = (struct multiboot2_tag_mmap *)tag;
                    struct multiboot2_mmap_entry *entry  = mmap_tag->entries;

                    while ((uintptr_t)entry < (uintptr_t)tag + tag->size) {
                        if (entry->type == MULTIBOOT2_MEMORY_AVAILABLE) {
                            // uint64_t region_end = entry->base_addr + entry->length;
                            /* if (region_end > max_addr_used) {
                                max_addr_used = region_end;
                                max_mem_used = *entry;
                            } */ // This choose the memory region to work with based on the address instead of the lenght of that address
                             
                            if (entry->length > max_mem_used.length) {
                                max_mem_used = *entry;
                                max_addr_used = entry->base_addr + entry->length;
                            }
                        }
                        entry = (multiboot2_mmap_entry_t *)((uintptr_t)entry +
                                                            mmap_tag->entry_size);
                    } break;
                case MULTIBOOT2_TAG_TYPE_END: goto exit;
            }
            tag = MULTIBOOT2_TAG_NEXT(tag);
        }
    }
exit:
    _entry(mbi_addr);
}