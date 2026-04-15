#pragma once
#include <stdint.h>



void free_blocks(const uint32_t *address, const uint32_t num_blocks);
void set_block(const uint32_t bit);
void *allocate_blocks(const uint32_t num_blocks);
void deinitialize_memory_region(const uint32_t base_address, const uint32_t size);
void initialize_memory_region(const uint32_t base_address, const uint32_t size);
void initialize_memory_manager(const uint32_t start_address, const uint32_t size);
int32_t find_first_free_blocks(const uint32_t num_blocks);
void set_block(const uint32_t bit);