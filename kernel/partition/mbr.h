#ifndef MBR_H
#define MBR_H
#include "partition.h"

struct mbr_partition_t
{
	uint8_t flags;
	uint32_t chr_start: 24;
	uint8_t type;
	uint32_t chr_end: 24;
	uint32_t lba;
	uint32_t size;
};

typedef struct mbr_partition_t mbr_t[4];

#endif
