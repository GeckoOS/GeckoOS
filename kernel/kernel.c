#include "drivers/mouse.h"
#include <drivers/tables/idt.h>
#include <drivers/tables/irq.h>
#include <drivers/tables/timer.h>
#include <drivers/vga.h>
#include <drivers/keyboard.h>
#include <drivers/drives.h>
#include <drivers/serial.h>
#include <layouts/kb_layouts.h>
#include <terminal/terminal.h>
#include <commands.h>
#include <colors.h>
#include <stdint.h>
#include <drivers/pci.h>
#include <mem/physical_mem.h>
#include <mem/paging.h>
#include <net/net.h>
#include <net/arp.h>

void process_input(unsigned char *buffer) {
    run_command(buffer, TERM_COLOR);
}

static void kmain();

__attribute__((section(".text.entry")))
void _entry() {
    serial_init();

    kalloc_init();

    /*
     * Bitmap goes at 0x500000 (512 bytes). Free region starts one page
     * later at 0x501000 so the allocator doesn't hand out its own bitmap
     * as the first allocation.
     */
    initialize_memory_manager(0x500000, 0x1000000);
    initialize_memory_region(0x501000, 0xFFF000);

    vga_clear(TERM_COLOR);
    printc("GeckoOS Version 2.0\n", TERM_COLOR);
    printc("Booted via GRUB/Multiboot2.\n", TERM_COLOR);

    set_layout(LAYOUTS[0]);

    printc("Enabling IDT...\n", VGA_COLOR_LIGHT_GREY);
    init_idt();
    printc("Enabling IRQ...\n", VGA_COLOR_LIGHT_GREY);
    irq_install();
    printc("Enabling Timer (50Hz)...\n", VGA_COLOR_LIGHT_GREY);
    timer_install();
    keyboard_install();
    mouse_init();
    terminal_init();
    timer_phase(50);

    printc("Testing interruption...\n", VGA_COLOR_LIGHT_GREY);
    asm volatile("int $0x3");
    printc("Test completed!\n", VGA_COLOR_LIGHT_GREY);

    printc("Enabling paging...\n", VGA_COLOR_LIGHT_GREY);
    if (!vmm_init()) {
        printc("vmm_init failed -- halting\n", VGA_COLOR_RED);
        for (;;) asm volatile("hlt");
    }

    drives_init();
    enumerate_pci();
    pci_detect_nics();

    net_init();
    arp_init();

    kmain();
}

static void kmain() {
    get_kdrive(0);

    unsigned char mount_cmd[] = "fsmount";
    run_command(mount_cmd, TERM_COLOR);
    printc("\n", TERM_COLOR);

    while (1) {
        printc("gecko> ", PROMPT_COLOR);
        unsigned char buff[512];
        input(buff, 512, TERM_COLOR);
        process_input(buff);
    }
}