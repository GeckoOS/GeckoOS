#include "ata.h"
#include "sys/io.h"
#include "sys/types.h"
#include "terminal/terminal.h"
#include "mem.h"
#include "drive.h"

#define ATA_SECTOR_COUNT 2
#define ATA_LOW 3
#define ATA_MID 4
#define ATA_HIGH 5
#define ATA_DRIVE_SELECT 6
#define ATA_STATUS 7

#define ATA_ERROR 0x1
#define ATA_DRQ 0x08
#define ATA_BUSY 0x80

#define ATA_MASTER 0xA0
#define ATA_SLAVE 0xB0

#define ATA_PRIMARY 0x1F0
#define ATA_SECONDARY 0x170

#define ATA_MAX_SECTOR_COUNT 256
#define ATA_SECTOR_SIZE 512

static void ata_wait_idle()
{
	while (inb(ATA_PRIMARY + ATA_STATUS) & ATA_BUSY);	
	while (inb(ATA_SECONDARY + ATA_STATUS) & ATA_BUSY);	
}
static void ata_drive_wait_idle( int drive )
{
	while (inb(drive + ATA_STATUS) & ATA_BUSY);	
}

static void ata_select_drive( int drive, int subdrive )
{
	outb(subdrive ? 0xB0 : 0xA0, drive + ATA_DRIVE_SELECT);
}

static void ata_read_drive( int drive, int subdrive, ssize_t size, void *data)
{
	outb(drive + ATA_DRIVE_SELECT, subdrive ? 0xB0 : 0xA0);
}

struct ata_identity_data
{

};

static ssize_t ata_kdrive_read( struct kdrive_t *this, ssize_t start, ssize_t num_bytes, void *output )
{	
	ssize_t num_sectors;
	ssize_t num_bytes_read;
	int i;
	uint16_t *aligned_output = output;

	num_sectors = (num_bytes + 511) / 512;
	if (num_sectors >= 256)
	{
		num_sectors = 0;
		num_bytes_read = ATA_MAX_SECTOR_COUNT * ATA_SECTOR_SIZE;

	}


	outb(this->user_data1 + ATA_DRIVE_SELECT, 
		(this->user_data2 ? 0xB0 : 0xA0) | 
		(0b11000000) | 
		((start>>24))%16 /* those bits 24-28 */
	);
	outb(this->user_data1 + ATA_LOW, start % 0xFF); 
	outb(this->user_data1 + ATA_MID, (start>>8) % 0xFF); 
	outb(this->user_data1 + ATA_HIGH, (start>>16) % 0xFF); 
	outb(this->user_data1 + ATA_SECTOR_COUNT, num_sectors); 
	outb(this->user_data1 + ATA_STATUS, 0x20);
	ata_drive_wait_idle(this->user_data1);
	for ( i = 0; i < num_bytes_read; i++ )
	{
		aligned_output[i] = inw(this->user_data1);
	}
	ata_drive_wait_idle(this->user_data1);

	return num_bytes_read;
}

static ssize_t ata_kdrive_write( struct kdrive_t *this, ssize_t start, ssize_t num_bytes, void *input )
{

}


static void ata_check_drive( int drive, int subdrive )
{
	struct kdrive_t kdrive;
	uint8_t status;
	uint16_t identify[256];

	


	outb(drive + ATA_DRIVE_SELECT, subdrive ? 0xB0 : 0xA0);
	outb(drive + ATA_SECTOR_COUNT, 0);
	outb(drive + ATA_LOW, 0);
	outb(drive + ATA_MID, 0);
	outb(drive + ATA_HIGH, 0);

	outb(drive + ATA_STATUS, 0xEC);



	ata_drive_wait_idle(drive);
	status = inb(drive+ATA_STATUS);
	if (!status)
		return;

	for (int i = 0; i < 256; i++)
		identify[i] = inw(drive);
	
	
	char model[41];
	for (int i = 0; i < 20; i++) {
		model[i*2]   = identify[27 + i] >> 8;
		model[i*2+1] = identify[27 + i] & 0xFF;
	}
	model[40] = '\0';

	memset(&kdrive, 0, sizeof(kdrive));
	memcpy(kdrive.name, model, 41);

	kdrive.sector_size = 512;
	kdrive.read = ata_kdrive_read;
	kdrive.write = ata_kdrive_write;
	kdrive.user_data1 = drive;
	kdrive.user_data2 = subdrive;

	kdrive_register(kdrive);
}



static void ata_detect_drives()
{

	ata_wait_idle();


	ata_check_drive(ATA_PRIMARY, 0);
	/*
	ata_check_drive(ATA_PRIMARY, 1);
	ata_check_drive(ATA_SECONDARY, 0);
	ata_check_drive(ATA_SECONDARY, 1);
	*/
}


void ata_master_init()
{
	ata_detect_drives();
};
