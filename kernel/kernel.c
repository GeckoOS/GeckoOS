// #include "kernel.h"
#include "commands.h" // Included by Ember2819: Adds commands
#include "drivers/drives.h"
#include "drivers/keyboard.h"
#include "drivers/tables/idt/idt.h"
#include "drivers/tables/irq/irq.h"
#include "drivers/tables/timer/timer.h"
#include "drivers/vga.h"
#include "gk/gk.h"
#include "layouts/kb_layouts.h"
#include "mem/mem.h"
#include "process/process.h"
#include <mem/physical_mem/physical_mem.h>
#include "terminal/terminal.h"
#include "users/users.h" // ember2819: user & permission system
#include <colors.h> // Added by MorganPG1 to centralise colors into one file
#include <stdint.h>
// this is just so that the process scheduler does not cause a triple fault (he
// does not like to be alone)
#define KERNEL_STATIC_MEM_AREA 4096 * 8
void process_input(unsigned char *buffer) { run_command(buffer, TERM_COLOR); }

static void kmain();

__attribute__((section(".text.entry"))) void _entry()
{

    kalloc_init();
    // Initialise display
    vga_clear(TERM_COLOR);

	initialize_memory_manager((uint32_t)0x0030000, KERNEL_STATIC_MEM_AREA);
	initialize_memory_region((uint32_t)0x0030000,KERNEL_STATIC_MEM_AREA );
	
    initialize_virtual_memory_manager();
    printc("----- GeckoOS v1.1 -----\n", TERM_COLOR);
    printc("Built by random people on the internet.\n", TERM_COLOR);

    // Setup keyboard layouts
    set_layout(LAYOUTS[0]);

    init_idt();
    irq_install();
    // DEFAULT QEMU GIVES 128  * 1024B (128KB)
    // need to keep in mind when paging so you do not get strange errors because
    // running out of memory
    // paging_init(10 * 1024);

    timer_install();
    keyboard_install();
    timer_phase(50);

    drives_init();
    users_init();
	

    printc("User system initialised. Default accounts: root / guest\n",
           VGA_COLOR_LIGHT_GREY);
	kmain();
	
}

static void kmain()
{
    get_kdrive(0);

    do_login_prompt();

    // process_t* process=create_process((uint32_t)HelloWorldProcess);

    while (1) {
        // Build the prompt: "username> "
        user_t *u = users_current();
        if (u) {
            uint8_t pcolor =
                (u->ring == RING_ADMIN) ? VGA_COLOR_LIGHT_RED : PROMPT_COLOR;
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
