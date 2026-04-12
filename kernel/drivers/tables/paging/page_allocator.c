
#include "page_allocator.h"

static inline void set_bit(uint32_t i) {
    page_bitmap[i / 8] |= (1 << (i % 8));
}

static inline void clear_bit(uint32_t i) {
    page_bitmap[i / 8] &= ~(1 << (i % 8));
}

static inline int test_bit(uint32_t i) {
    return page_bitmap[i / 8] & (1 << (i % 8));
}

void* alloc_page(void)
{
    for (uint32_t i = 0; i < MAX_PAGES; i++) {
        if (!test_bit(i)) {
            set_bit(i);
            return (void*)(i * PAGE_SIZE);
        }
    }
    return (void*)0; // out of memory
}

void free_page(void* addr)
{
    uint32_t page = (uint32_t)addr / PAGE_SIZE;
    clear_bit(page);
}

void map_page(uint32_t* pd, uint32_t virt, uint32_t phys, uint32_t flags)
{
    uint32_t dir = virt >> 22;
    uint32_t tbl = (virt >> 12) & 0x3FF;

    uint32_t* table = (uint32_t*)(pd[dir] & ~0xFFF);

    table[tbl] = phys | flags;
}
