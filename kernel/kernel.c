//#include "kernel.h"
#include "drivers/vga.h"
#include "drivers/keyboard.h"
#include "layouts/kb_layouts.h"
#include "mem.h"
#include "terminal/terminal.h"
#include "drivers/drives/drive.h"

void drives_init();
struct kdrive_t *get_drive( int i );

__attribute__((section(".text.entry"))) // Add section attribute so linker knows this should be at the start
void _entry()
{
	terminal_clear(VGA_COLOR_BLACK);
	termprint("hi\n", VGA_COLOR_LIGHT_GREY);

	drives_init();
	struct kdrive_t *drive = get_drive(0);
	char data[512] = "not inited yet";
	drive->read(drive, 61, 512, &data);
	for (int i = 0; i < 512; i++)
	{
		putchar(data[i], VGA_COLOR_LIGHT_GREY);
	}

	for (;;)
	{

	}
}
