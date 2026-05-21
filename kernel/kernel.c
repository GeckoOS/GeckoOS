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
     * Place the physical allocator bitmap immediately after BSS, page-aligned.
     * Using _bss_end from the linker script means this is always safe no matter
     * how large BSS grows — previously it was hardcoded to 0x500000 which BSS
     * was overrunning (console_history alone is ~2.5MB in BSS).
     */
    extern uint64_t _bss_end;
    uint64_t bitmap_start = ((uint64_t)&_bss_end + 0xFFF) & ~0xFFFULL;
    uint64_t free_start   = bitmap_start + 0x1000;
    uint64_t mem_end      = 0x1000000;

    initialize_memory_manager(bitmap_start, mem_end);
    initialize_memory_region(free_start, mem_end - free_start);

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