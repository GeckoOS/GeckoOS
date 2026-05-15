#include <drivers/tables/idt.h>
#include <drivers/tables/irq.h>
#include <drivers/tables/timer.h>
#include <drivers/vga.h>
#include <drivers/keyboard.h>
#include <drivers/drives.h>
#include <layouts/kb_layouts.h>
#include <terminal/terminal.h>
#include <commands.h>
#include <colors.h>
#include <stdint.h>
#include <drivers/pci.h>

void process_input(unsigned char *buffer) {
    run_command(buffer, TERM_COLOR);
}

static void kmain();

__attribute__((section(".text.entry")))
void _entry() {
    kalloc_init();

    vga_clear(TERM_COLOR);
    printc("----- GeckoOS v1.1 -----\n", TERM_COLOR);
    printc("Built by random people on the internet.\n", TERM_COLOR);
    printc("Booted via GRUB/Multiboot2.\n", TERM_COLOR);

    set_layout(LAYOUTS[0]);

    printc("Enabling IDT...\n", VGA_COLOR_LIGHT_GREY);
    init_idt();
    printc("Enabling IRQ...\n", VGA_COLOR_LIGHT_GREY);
    irq_install();
    printc("Enabling Timer (50Hz)...\n", VGA_COLOR_LIGHT_GREY);
    timer_install();
    keyboard_install();
    timer_phase(50);

    printc("Testing interruption...\n", VGA_COLOR_LIGHT_GREY);
    asm volatile("int $0x3");
    printc("Test completed!\n", VGA_COLOR_LIGHT_GREY);

    drives_init();
    enumerate_pci();

    kmain();
}

static void kmain() {
    get_kdrive(0);

    while (1) {
        printc("gecko> ", PROMPT_COLOR);
        unsigned char buff[512];
        input(buff, 512, TERM_COLOR);
        process_input(buff);
    }
}