#include <boot/multiboot2.h>
#include <stdint.h>

#define VGA_COLOR_BLACK     0
#define VGA_COLOR_LIGHT_RED 12

#define MULTIBOOT1_BOOTLOADER_MAGIC 0x2BADB002

uint64_t g_mbi_addr         = 0;
uint64_t g_multiboot2_magic = 0;

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

    _entry();

    for (;;) { __asm__ volatile ("cli; hlt"); }
}