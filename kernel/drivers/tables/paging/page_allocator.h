#pragma once

#include <stdint.h>

#define PAGE_SIZE 4096
#define MAX_PAGES 32768  // supports up to 128MB RAM (32768 * 4KB)

static uint8_t page_bitmap[MAX_PAGES / 8];
///@b allocate a page
///@return returns a pointer to the page
void* alloc_page(void);
//free a page
void free_page(void* address);

void map_page(uint32_t* pd, uint32_t virt, uint32_t phys, uint32_t flags);