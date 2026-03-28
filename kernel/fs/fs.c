#include "fs.h"

struct drive_fs_t *drive_open( struct kdrive_t *drive )
{
	struct partition_t part;
	
	part.type = FS_FAT16;
	part.lba = 0;
	part.size = SIZE_MAX;
	return partition_open(&part);
}

struct drive_fs_t *partition_open( struct partition_t *partition )
{
	switch (partition->type)
	{
	case FS_FAT16:
		//return fat16_mount(partition);
	}
}
