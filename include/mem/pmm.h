#ifndef PMM_H
#define PMM_H
#include "stdbool.h"
#include <stdint.h>

// https://codeberg.org/pizzuhh/AxiomOS/src/branch/main/src/kernel/include/memory/pmm.h

#define MEMORY_BLOCK_SIZE 	4096
#define BLOCKS_PER_BYTE 	8
#define SMAP_ADDRESS 0x5000
#define VESA_ADDRESS 0x9000

extern uint32_t *memory_bitmap;
extern uint32_t used_blocks;
extern uint32_t free_blocks;
extern uint32_t max_blocks;

typedef struct {
	uint64_t address;
	uint64_t length;
	uint32_t type; // 1- (U)sable; 2 - (R)eserved; 3 - (A)CPI reclaimable memomry; 4 - (A)CPI NVS Memory; 5 - (B)ad memory
	uint32_t acpi;
} __attribute__((packed)) SMAP_t;

extern uint32_t number_smap_entries;
extern SMAP_t *entries;

void pmm_set_block(uint32_t bit);
void pmm_unset_block(uint32_t bit);

int pmm_find_free_blocks(uint32_t blocks);
void pmm_init(uint32_t start_address, uint32_t size);
void pmm_map_region(uint32_t address, uint32_t size);
void pmm_unmap_region(uint32_t address, uint32_t size);

void *pmm_alloc_blocks(uint32_t blocks);
void pmm_free_blocks(void *start_addr, uint32_t blocks_size);

#endif
