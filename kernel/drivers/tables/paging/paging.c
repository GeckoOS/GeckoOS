
#include "paging.h"
#include <stdint.h>

#define PAGE_SIZE 4096
#define PAGE_TABLE_ENTRIES 1024
#define PAGE_DIR_ENTRIES 1024

uint32_t __attribute__((aligned(PAGE_SIZE))) page_directory[PAGE_DIR_ENTRIES];


void paging_init(uint32_t physmem_kb) {
    uint32_t pages = physmem_kb / 4;
    uint32_t tables = (pages + PAGE_TABLE_ENTRIES - 1) / PAGE_TABLE_ENTRIES;

    static uint32_t page_tables[1024][PAGE_TABLE_ENTRIES]
        __attribute__((aligned(PAGE_SIZE)));

    uint32_t page = 0;

    for (uint32_t i = 0; i < tables; i++) {
        for (uint32_t j = 0; j < PAGE_TABLE_ENTRIES; j++) {
            if (page >= pages) {
                page_tables[i][j] = 0;
            } else {
                page_tables[i][j] = (page * PAGE_SIZE) | 3; // Present + RW
                page++;
            }
        }
        page_directory[i] = ((uint32_t)&page_tables[i]) | 3; // Present + RW
    }

    for (uint32_t i = tables; i < PAGE_DIR_ENTRIES; i++) {
        page_directory[i] = 0;
    }

    for (uint32_t i = 0; i < (16 * 1024 * 1024) / PAGE_SIZE; i++) {
        uint32_t dir = i / PAGE_TABLE_ENTRIES;
        uint32_t tbl = i % PAGE_TABLE_ENTRIES;
        page_tables[dir][tbl] = (i * PAGE_SIZE) | 3;
        page_directory[dir] = (uint32_t)&page_tables[dir] | 3;
    }

    // Enable paging
    paging_enable((uint32_t)page_directory);
    uint32_t temp =page_tables[0][1];
    page_tables[0][1] = (0x3000) | 3; // VA 0x1000 → PA 0x3000
    *(int *)0x1000 = 55;

    if (*(int *)0x3000 == 55) {
        print("Paging remap test passed\n");
    }
    page_tables[0][1] =temp;
    
}