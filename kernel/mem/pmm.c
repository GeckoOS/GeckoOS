#include "drivers/vga.h"
#include <mem/pmm.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

// https://codeberg.org/pizzuhh/AxiomOS/src/branch/main/src/kernel/include/memory/pmm.c

uint32_t *memory_bitmap = NULL;
uint32_t max_blocks = 0;
uint32_t used_blocks = 0;

uint32_t number_smap_entries;
SMAP_t *entries;

void pmm_set_block(uint32_t bit) {
	memory_bitmap[bit/32] |= (1 << (bit % 32));
}

void pmm_unset_block(uint32_t bit) {
	memory_bitmap[bit/32] &= ~(1 << (bit % 32));
}

int pmm_find_free_blocks(uint32_t blocks) {
	if (blocks == 0) return -1;

	for (uint32_t i = 0; i < max_blocks / 32; i++) {
		if (memory_bitmap[i] != 0xFFFFFFFF) {
			for (uint32_t j = 0; j < 32; j++) {
				int bit = (1 << j);
				if (!(memory_bitmap[i] & bit)) {
					for (uint32_t count = 0, free_blocks = 0; count < blocks; count++) {
						if ((j + count > 31) && (j + 1 <= max_blocks / 32)) {
							if (!(memory_bitmap[i+1] & (1 << ((j + count) - 32))))
								free_blocks++;
						} else {
								if (!(memory_bitmap[i] & (1 << (j + count))))
									free_blocks++;
							}
							if (free_blocks == blocks)
								return i * 32 + j;
						}
					}
				}
			}
		}
	return -1;
}

void pmm_init(uint32_t start_address, uint32_t size) {
	entries = (SMAP_t*)SMAP_ADDRESS + 0x4;
	number_smap_entries = *(uint32_t*)SMAP_ADDRESS;

	memory_bitmap = (uint32_t*)start_address;
	max_blocks = size / MEMORY_BLOCK_SIZE;
	used_blocks = max_blocks;
	memset(memory_bitmap, 0xFF, max_blocks / BLOCKS_PER_BYTE);
}

void pmm_map_region(uint32_t address, uint32_t size) {
	uint32_t align = address / MEMORY_BLOCK_SIZE;
	uint32_t num_blocks = size / MEMORY_BLOCK_SIZE;

	for (; num_blocks > 0; num_blocks--) {
		pmm_unset_block(align++);
		used_blocks--;
	}
	pmm_set_block(0);
}

void pmm_unmap_region(uint32_t address, uint32_t size) {
	uint32_t align = address / MEMORY_BLOCK_SIZE;
	uint32_t num_blocks = size / MEMORY_BLOCK_SIZE;

	for (; num_blocks > 0; num_blocks--) {
		pmm_set_block(align++);
		used_blocks++;
	}

}

void *pmm_alloc_blocks(uint32_t blocks) {
	if ((max_blocks - used_blocks) <= blocks) return NULL;
	int32_t start_block = pmm_find_free_blocks(blocks);
	if (start_block == -1) return NULL;
	for (uint32_t i = 0; i < blocks; i++) {
		pmm_set_block(start_block + i);
	}
	used_blocks += blocks;
	return (void*)(start_block * MEMORY_BLOCK_SIZE);

}
void pmm_free_blocks(void *start_addr, uint32_t blocks_size) {
	uint32_t start_block = (((uint32_t)start_addr) / MEMORY_BLOCK_SIZE);
	for (uint32_t i = 0; i < blocks_size; i++) {
		pmm_unset_block(start_block + i);
	}
	used_blocks -= blocks_size;
}
