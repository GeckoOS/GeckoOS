//#include "kernel.h"
#include "drivers/tables/idt/idt.h"
#include "drivers/tables/idt/idt.h"
#include "drivers/tables/irq/irq.h"
#include "drivers/tables/timer/timer.h"
#include "drivers/vga.h"
#include "drivers/keyboard.h"
#include "layouts/kb_layouts.h"
#include "terminal/terminal.h"
#include "drivers/drives/drive.h"
#include "colors.h"
#include <stdint.h>

void drives_init();
struct kdrive_t *get_drive( int i );

static void kmain();

__attribute__((section(".text.entry"))) // Add section attribute so linker knows this should be at the start
void _entry() {
	vga_clear(TERM_COLOR);
	termprint("----- COMMUNITY OS v1.0 -----\n", TERM_COLOR);
	termprint("Built by random people on the internet.\n", TERM_COLOR);
	termprint("Use help to see available commands.\n", TERM_COLOR);

	// Setup keyboard layouts
	set_layout(LAYOUTS[0]);

	termprint("Enabling IDT...\n", VGA_COLOR_LIGHT_GREY);
	init_idt();
	termprint("Enabling IRQ...\n", VGA_COLOR_LIGHT_GREY);
	irq_install();
	termprint("Enabling Timer and setting it to 50Hz...\n", VGA_COLOR_LIGHT_GREY);
	timer_install();
	timer_phase(50);
	termprint("Testing interruption...\n", VGA_COLOR_LIGHT_GREY);
	asm volatile("int $0x3");
	termprint("Test completed!\n", VGA_COLOR_LIGHT_GREY);
	kmain(); // _entry will be the initialization
}

static void kmain()
{
	// malloc(938);
	// outb(0x64, 0xfe); // Reboots the machine? (It acts weird in QEMU, but it reboots at least)
	//
	//
	termprint("Testing drives\n", VGA_COLOR_LIGHT_GREY);
	drives_init();
	struct kdrive_t *drive = get_drive(0);
	char data[512] = "This should not be seen\n";
	drive->read(drive, 0, 1, &data);
	for (int i = 0; i < 512; i++)
	{
		putchar(data[i], VGA_COLOR_LIGHT_GREY);
	}

	for (;;);
}
