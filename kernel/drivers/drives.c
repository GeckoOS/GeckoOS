#include <drivers/drives.h>
#include <mem.h>
#include <drivers/ata.h>
#include <terminal/terminal.h>
#include <partition/mbr.h>

static struct kdrive_t drives[32];

static void load_parameters( struct kdrive_t *drive )
{
	unsigned char sector[512];
	mbr_t mbr;
	int i;
	drive->read((void*)drive, 0, 1, sector);

	if (sector[510]==0x55 && sector[511]==0xaa)
	{
		memcpy(mbr, sector+446, 64);
		drive->partitions.count = 4;
		drive->partitions.type = PARTITION_MBR;
		drive->partitions.partitions = kmalloc(4 * sizeof(struct partition_t));
		for ( i = 0; i < 4; i++ )
		{
			drive->partitions.partitions[i].type = FS_FAT32;
			drive->partitions.partitions[i].lba = mbr[i].lba;
			drive->partitions.partitions[i].size = mbr[i].size;
		}
	}
	else
	{
		drive->partitions.count = 1;
		drive->partitions.type = PARTITION_NONE;
		drive->partitions.partitions = kmalloc(sizeof(struct partition_t));
		drive->partitions.partitions[0].type = FS_FAT32;
		drive->partitions.partitions[0].lba  = 0;
		drive->partitions.partitions[0].size = 0;
	}
};

void register_kdrive(struct kdrive_t *drive)
{
	int i;
	for ( i = 0; i < 32; i++ )
	{
		if (drives[i].sector_size)
			continue;
		load_parameters(drive);
		drives[i] = *drive;
		break;
	}
};

struct kdrive_t *get_kdrive( int i )
{
	if (drives[i].read == 0)
		return NULL;
	return &drives[i];
}

void drives_init()
{
	memset(drives, 0, sizeof(drives));
	ata_init();
}
