//#include "kernel.h"
#include "drivers/vga.h"
#include "drivers/keyboard.h"
#include "layouts/kb_layouts.h"
#include "terminal/terminal.h"
#include "drivers/drives/drive.h"
#include "colors.h"

void drives_init();
struct kdrive_t *get_drive( int i );

static void kmain();

__attribute__((section(".text.entry"))) // Add section attribute so linker knows this should be at the start
void _entry() {
	// Initialise display.
	terminal_clear(VGA_COLOR_BLACK);
	kmain(); // _entry will be the initialization
}

static void kmain()
{
	// malloc(938);
	// outb(0x64, 0xfe); // Reboots the machine? (It acts weird in QEMU, but it reboots at least)
	//
	drives_init();
	struct kdrive_t *drive = get_drive(0);
	char data[512] = "not inited yet";
	drive->read(drive, 0, 512, &data);
	for (int i = 0; i < 512; i++)
	{
		putchar(data[i], VGA_COLOR_LIGHT_GREY);
	}

	for (;;);
}
