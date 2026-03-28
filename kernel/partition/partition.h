#ifndef PARTITION_H
#define PARTITION_H
#include "stdint.h"

enum partition_fs_t
{
	FS_NONE,
	FS_FAT12,
	FS_FAT16,
	FS_FAT32,
};

struct partition_t
{
	enum partition_fs_t type;
	uint64_t lba;
	uint64_t size;
};

enum partition_type_t
{
	PARTITION_NONE,
	PARTITION_MBR,
	PARTITION_GPT,
};

struct partition_table_t
{
	enum partition_type_t type;
	uint8_t count;
	struct partition_t *partitions;
};



#endif
