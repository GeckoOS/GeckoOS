#include "drives.h"
#include "../mem.h"
#include "ata.h"
#include "../terminal/terminal.h"
#include "../drivers/drives.h"
#include "../partition/mbr.h"

static struct kdrive_t drives[32];

static void load_parameters( struct kdrive_t *drive )
{
	unsigned char sector[512];
	mbr_t mbr;
	int i;
	drive->read((void*)drive, 0, 1, sector);

	/* mbr signature */
	if (sector[510]==0x55 && sector[511]==0xaa)
	{
		memcpy(mbr, sector+446, 64);

		drive->partitions.count = 4;
		drive->partitions.type = PARTITION_MBR;
		drive->partitions.partitions = malloc(sizeof(struct partition_t));
		for ( i = 0; i < 4; i++ )
		{
			drive->partitions.partitions[i].type = FS_FAT16;
			drive->partitions.partitions[i].lba = mbr[i].lba;
			drive->partitions.partitions[i].size = mbr[i].size;

		}

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
