
#include "stdint.h"
#include "../headers/sys/io.h"

struct mbr_partiton {
	uint32_t flags;
	uint32_t type;
	uint32_t lda;
	uint32_t size;
} __attribute__((packed));

static struct mbr_partiton *partitons = (struct mbr_partiton*)(0x7DBE);

static struct mbr_partiton bootable_drives[32];
static int num_bootable_drives;

#define VGA_TEXT_ADDR 0xb8000
#define VGA_GRAPHICS_ADDR 0xa0000
#define VGA_TEXT_WIDTH 80
#define VGA_TEXT_HEIGHT 25

#define VGA_FG_COLOR 7
#define VGA_BG_COLOR 0

#define PS2_KB_BUFF_SIZE 512
#define KEYBOARD_DATA_PORT 0x60
#define KEYBOARD_COMMAND_PORT 0x64
#define KEYBOARD_STATUS_PORT 0x64

static struct vga_char_t
{
	char character;
	uint8_t color;
} * const display = (void*)VGA_TEXT_ADDR;

void clean_display()
{
	int i = 0;
	for (i = 0; i < VGA_TEXT_WIDTH*VGA_TEXT_HEIGHT; i++)
	{
		display[i].character = '\0';
		display[i].color = VGA_FG_COLOR;
	}
};

void display_write_text( const char *text, int x, int y, char selected )
{
	uint8_t color;
	int i;

	color = 7;
	i = 0;
	if (selected)
		color = 0x70;
		
	while (*text)
	{
		display[x+y*VGA_TEXT_WIDTH+i].character = *text;
		display[x+y*VGA_TEXT_WIDTH+i].color = color;
		text++;
		i++;
	}
}

void render_partitions( int selected )
{
	int i;

	for ( i = 0; i < num_bootable_drives; i++ )
	{
		display_write_text("something bootable",0,i, selected == i);
	}
}

#define ATA_SECTOR_COUNT 2
#define ATA_LOW 3
#define ATA_MID 4
#define ATA_HIGH 5
#define ATA_DRIVE_SELECT 6
#define ATA_STATUS 7
#define ATA_COMMAND 7

#define ATA_ERROR 0x1
#define ATA_DRQ 0x08
#define ATA_BUSY 0x80

#define ATA_MASTER 0xA0
#define ATA_SLAVE 0xB0

#define ATA_PRIMARY 0x1F0
#define ATA_SECONDARY 0x170

#define ATA_MAX_SECTOR_COUNT 256
#define ATA_SECTOR_SIZE 512

void vga_put_number(int row, int col, int number, uint8_t color) {
	volatile uint16_t* vga = (volatile uint16_t*)VGA_TEXT_ADDR;
	char buffer[12]; // enough for int32
	int i = 0;

	// Convert number to string (basic itoa)
	if (number == 0) {
		buffer[i++] = '0';
	} else {
		int num = number;
		char temp[12];
		int j = 0;
		int negative = 0;

		if (num < 0) {
			negative = 1;
			num = -num;
		}

		while (num > 0) {
			temp[j++] = '0' + (num % 10);
			num /= 10;
		}

		if (negative) temp[j++] = '-';

		// reverse
		while (j > 0) {
			buffer[i++] = temp[--j];
		}
	}
	buffer[i] = '\0';

	// Write string to VGA memory
	for (int k = 0; buffer[k] != '\0'; k++) {
		uint16_t value = ((uint16_t)color << 8) | buffer[k];
		vga[row * VGA_TEXT_WIDTH + col + k] = value;
	}
}

void load_kernel( int kernel )
{
	uint16_t *data;
	uint32_t start;
	uint32_t remaining_sectors;
	uint32_t read_sectors;
	uint32_t sectors_to_read;
	int i;

	clean_display();

	data = (uint16_t*)0x100000;
	start = bootable_drives[kernel].lda;
	read_sectors = 0;
	remaining_sectors = bootable_drives[kernel].size;
	sectors_to_read = 0;


	while (remaining_sectors!=0)
	{
		if (remaining_sectors >= 256)
			sectors_to_read = 256;
		else
			sectors_to_read = remaining_sectors;
		
		outb(ATA_PRIMARY + ATA_DRIVE_SELECT, 
				ATA_MASTER | 
				(0b11000000) | 
				((start>>24))&0xF /* those bits 24-28 */
		    );

		outb(ATA_PRIMARY + ATA_LOW, start & 0xFF); 
		outb(ATA_PRIMARY + ATA_MID, (start>>8) & 0xFF); 
		outb(ATA_PRIMARY + ATA_HIGH, (start>>16) & 0xFF); 
		outb(ATA_PRIMARY + ATA_SECTOR_COUNT, sectors_to_read); 
		outb(ATA_PRIMARY + ATA_COMMAND, 0x20);
		while (inb(ATA_PRIMARY + ATA_STATUS) & ATA_BUSY);	

		for ( i = 0; i < sectors_to_read*256; i++ )
		{
			data[i+read_sectors*256] = inw(ATA_PRIMARY);
		}

		for ( i = 0; i < 1024; i++ )
		{
		}


		read_sectors += sectors_to_read;
		start += sectors_to_read;
		remaining_sectors-=sectors_to_read;

		while (inb(ATA_PRIMARY + ATA_STATUS) & ATA_BUSY);	

	}

	asm volatile("jmp 0x100000");


};

__attribute__((section(".text.entry"))) // Add section attribute so linker knows this should be at the start
void _entry()
{
	uint8_t scancode;
	uint8_t scancode2;
	int i;
	int selected_partiton;

	num_bootable_drives = 0;
	selected_partiton = 0;

	clean_display();
	// we ignore first one
	for (i = 0; i < 4; i++ )
	{
		if (partitons[i].flags == 0x80)
			bootable_drives[num_bootable_drives++] = partitons[i];
	}

	outb(0x3D4, 0x0A);
	outb(0x3D5, 0x20);
	
	render_partitions(selected_partiton);


	for (;;)
	{


		while (!(inb(KEYBOARD_STATUS_PORT) & 1)) {
			asm volatile("pause");
		}
		scancode = inb(KEYBOARD_DATA_PORT);
		if (scancode == 0xE0) {
			while (!(inb(KEYBOARD_STATUS_PORT) & 1)) {
				asm volatile("pause");
			}
			scancode2 = inb(KEYBOARD_DATA_PORT);
			if (scancode2 == 0x48)
			{
				selected_partiton--;
				selected_partiton = selected_partiton == -1 ? 0 : selected_partiton;

			}
			if (scancode2 == 0x50)
			{
				selected_partiton++;
				selected_partiton = selected_partiton == num_bootable_drives ? num_bootable_drives - 1 : selected_partiton;
			}
		}
		if (scancode == 0x1C) {
			while (!(inb(KEYBOARD_STATUS_PORT) & 1)) {
				asm volatile("pause");
			}
			scancode2 = inb(KEYBOARD_DATA_PORT);
			if (scancode2 == 0x9C)
			{
				load_kernel(selected_partiton);
			}
		}
		render_partitions(selected_partiton);
	}
}
