//#include "kernel.h"
#include "drivers/tables/idt/idt.h"
#include "drivers/tables/idt/idt.h"
#include "drivers/tables/irq/irq.h"
#include "drivers/tables/timer/timer.h"
#include "drivers/vga.h"
#include "drivers/keyboard.h"
#include "drivers/drives.h"
#include "layouts/kb_layouts.h"
#include "paging/paging.h"
#include "terminal/terminal.h"
#include "commands.h" // Included by Ember2819: Adds commands
#include "colors.h" // Added by MorganPG1 to centralise colors into one file
#include <stdint.h>

// Ember2819: Add command functionality
void process_input(unsigned char *buffer) {
    run_command(buffer, TERM_COLOR);
}

static void kmain();

__attribute__((section(".text.entry"))) // Add section attribute so linker knows this should be at the start
void _entry() {
    // Initialise display.
    vga_clear(TERM_COLOR);
    printc("----- COMMUNITY OS v1.0 -----\n", TERM_COLOR);
    printc("Built by random people on the internet.\n", TERM_COLOR);
    printc("Use help to see available commands.\n", TERM_COLOR);

    // Setup keyboard layouts
    set_layout(LAYOUTS[0]);

    printc("Enabling IDT...\n", VGA_COLOR_LIGHT_GREY);
    init_idt();
    printc("Enabling IRQ...\n", VGA_COLOR_LIGHT_GREY);
    irq_install();
    printc("Enabling Timer and setting it to 50Hz...\n", VGA_COLOR_LIGHT_GREY);
    timer_install();
    timer_phase(50);
    printc("Testing interruption...\n", VGA_COLOR_LIGHT_GREY);
    asm volatile("int $0x3");
    printc("Test completed!\n", VGA_COLOR_LIGHT_GREY);
    initialise_paging();
    drives_init();
    kmain(); // _entry will be the initialization
}

static void kmain()
{
    // malloc(938); Idk if it works tbh
    // outb(0x64, 0xfe); // Reboots the machine? (It acts weird in QEMU, but it reboots at least)
    get_kdrive(0);

    while (1) {    // Shell loop
        // Prints shell prompt
        printc("> ", PROMPT_COLOR);
        
        //Obtains and processes the user input

        unsigned char buff[512];
        input(buff, 512, TERM_COLOR);
        process_input(buff);
    }

    //asm volatile ("hlt");
}
