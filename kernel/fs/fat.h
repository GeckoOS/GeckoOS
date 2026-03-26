#ifndef FAT_H
#define FAT_H
#include "stdint.h"

struct fatBPB
{
	uint8_t jump_boot[3];
	uint8_t oem_name[8];
	uint16_t bytes_per_sector;
	uint8_t sectors_per_cluster;
	uint8_t reserved_sector_count;
	uint8_t fat_count;
	uint16_t root_entries_count;
	uint16_t total_sector_count16;
	uint8_t media;
	uint16_t fatsz16;
	uint16_t sectors_per_track;
	uint16_t num_heads;
	uint32_t hidden_sector_count;
	uint32_t total_sector_count32;
} __attribute__((packed));

#endif
