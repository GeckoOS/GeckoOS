#include <fs/fat32.h>
#include <fs/fs.h>

struct drive_fs_t *fs_drive_open( struct kdrive_t *drive )
{
	struct partition_t part;

	part.type = FS_FAT32;
	part.lba = 0;
	part.size = SIZE_MAX;
	return fs_partition_open(drive, &part);
}

struct drive_fs_t *fs_partition_open( struct kdrive_t *drive, struct partition_t *partition )
{
	switch (partition->type)
	{
	case FS_FAT32:
		return fat32_drive_open(drive, partition);
	case FS_NONE:
	case FS_FAT12:
	case FS_FAT16:
		break;
	}
	return 0;
}

void fs_free_entries( struct fs_entries_t *entries )
{
	/* we do not have free lol, let that sink in */
}