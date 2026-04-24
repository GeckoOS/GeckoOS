//#include "kernel.h"
#include "drivers/pci.h"
#include "terminal/printf.h"
#include <drivers/tables/gdt/gdt.h>
#include <drivers/tables/idt/idt.h>
#include <drivers/tables/idt/idt.h>
#include <drivers/tables/irq/irq.h>
#include <drivers/tables/timer/timer.h>
#include <drivers/vga.h>
#include <drivers/keyboard.h>
#include <drivers/drives.h>
#include <layouts/kb_layouts.h>
#include <mem.h>
#include <multiboot.h>
#include <ports.h>
#include <terminal/terminal.h>
#include <commands.h>       // Included by Ember2819: Adds commands
#include <colors.h>         // Added by MorganPG1 to centralise colors into one file
#include <users.h>    // ember2819: user & permission system
#include <stdint.h>
#include <users.h>
#include <cpuid.h>
#include <mem/vmm.h>
#include <mem/pmm.h>

void process_input(unsigned char *buffer) {
    run_command(buffer, TERM_COLOR);
}

static void kmain(multiboot_info_t* multiboot);

__attribute__((section(".text")))
void kernel_main(multiboot_info_t* multiboot) {
    if (!multiboot) return;

    pmm_init(_kernel_end, multiboot->mem_upper * multiboot->mem_lower);
    pmm_map_region(_kernel_end, multiboot->mem_upper * multiboot->mem_lower);
    init_virtual_memory_manager();

    // Initialise display
    vga_clear(TERM_COLOR);
    printc("----- GeckoOS v1.2 -----\n", TERM_COLOR);
    printc("Built by random people on the internet.\n", TERM_COLOR);

    // Setup keyboard layouts
    set_layout(LAYOUTS[0]);

    init_gdt();
    init_idt();
    irq_install();
    timer_install();
    keyboard_install();
    timer_phase(50);
    drives_init();
    users_init();
    enumerate_pci();

    printc("User system initialised. Default accounts: root / guest\n", VGA_COLOR_LIGHT_GREY);

    kmain(multiboot);
}

static void kmain(multiboot_info_t* multiboot)
{
    get_kdrive(0);

    printkf("%d\n", multiboot->mem_upper + multiboot->mem_lower);

    do_login_prompt();

    // int ebx, unused;
    // __cpuid(0, unused, ebx, unused, unused);

    while (1) {
        // Build the prompt: "username> "
        user_t *u = users_current();
        if (u) {
            uint8_t pcolor = (u->ring == RING_ADMIN) ? VGA_COLOR_LIGHT_RED : PROMPT_COLOR;
            printc(u->name, pcolor);
            printc("> ", pcolor);
        } else {
            // Shouldn't reach here, but be safe
            printc("> ", PROMPT_COLOR);
        }

        unsigned char buff[512];
        input(buff, 512, TERM_COLOR);
        process_input(buff);
    }
}
