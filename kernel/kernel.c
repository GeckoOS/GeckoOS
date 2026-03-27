//#include "kernel.h"
#include "drivers/tables/idt/idt.h"
#include "drivers/tables/idt/idt.h"
#include "drivers/tables/irq/irq.h"
#include "drivers/tables/timer/timer.h"
#include "drivers/vga.h"
#include "drivers/keyboard.h"
#include "layouts/kb_layouts.h"
#include "mem.h"
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
    printf("----- COMMUNITY OS v1.0 -----\n", TERM_COLOR);
    printf("Built by random people on the internet.\n", TERM_COLOR);
    printf("Use help to see available commands.\n", TERM_COLOR);

    // Setup keyboard layouts
    set_layout(LAYOUTS[0]);

    printf("Enabling IDT...\n", VGA_COLOR_LIGHT_GREY);
    init_idt();
    printf("Enabling IRQ...\n", VGA_COLOR_LIGHT_GREY);
    irq_install();
    printf("Enabling Timer and setting it to 50Hz...\n", VGA_COLOR_LIGHT_GREY);
    timer_install();
    timer_phase(50);
    printf("Testing interruption...\n", VGA_COLOR_LIGHT_GREY);
    asm volatile("int $0x3");
    printf("Test completed!\n", VGA_COLOR_LIGHT_GREY);
    // initialise_paging(); // Crashes
    kmain(); // _entry will be the initialization
}

static void kmain()
{
    // malloc(938); Idk if it works tbh
    // outb(0x64, 0xfe); // Reboots the machine? (It acts weird in QEMU, but it reboots at least)

    while (1) {    // Shell loop
        // Prints shell prompt
        printf("> ", PROMPT_COLOR);
        
        //Obtains and processes the user input

        unsigned char buff[512];
        input(buff, 512, TERM_COLOR, 1);
        process_input(buff);
    }

    //asm volatile ("hlt");
}
