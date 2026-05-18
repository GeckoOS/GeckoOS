#pragma once
#include <stdint.h>

void  free_blocks(const void *address, uint32_t num_blocks);
void  set_block(uint32_t bit);
void *allocate_blocks(uint32_t num_blocks);
void  deinitialize_memory_region(uint64_t base_address, uint64_t size);
void  initialize_memory_region(uint64_t base_address, uint64_t size);
void  initialize_memory_manager(uint64_t start_address, uint64_t size);
int32_t find_first_free_blocks(uint32_t num_blocks);