#pragma once
/**
 * Defines functions for paging and virtual memory mapping
 *
 */
#include <stdint.h>

#define PAGE_SIZE            4096
#define PAGES_PER_TABLE      1024
#define TABLES_PER_DIRECTORY 1024
#define KERNEL_ADDRESS 	     0x100000
#define PD_INDEX(address) ((address) >> 22)
#define PT_INDEX(address) (((address) >> 12) & 0X3FF) // Max index 1023=0x3ff
#define PAGE_PHYS_ADRESS(dir_entry) (*(dir_entry) & ~0xFFF) // CLear lowest 12 bits only return frame adress

#define SET_ATTRIBUTE(entry, atr)   (*entry |= atr)
#define CLEAR_ATTRIBUTE(entry, atr) (*entry &= ~atr)
#define TEST_ATTRIBUTE(entry, atr)  (*entry & atr)
#define SET_FRAME(entry, address) ((*entry = (*entry & ~0x7FFFF000)) |address) // Only set adress frame not flags

typedef uint32_t pd_entry_t;
typedef uint32_t pt_entry_t; // page table entry
typedef uint32_t pt_entry_tpd_entry_t; // page directory entry
typedef uint32_t physical_address_t;
typedef uint32_t virtual_address_t;
// PTE entry
typedef enum {
    PTE_PRESENT        = 0x01, // bit 1
    PTE_READ_AND_WRITE = 0x02, // bit2
    PTE_USER           = 0x04, // bit 3
    PTE_WRITE_TROUGH   = 0X08, // bit 4 ...
    PTE_CACHE_DISABLE  = 0X10,
    PTE_ACCESSED       = 0X20,
    PTE_DIRTY          = 0X40,
    PTE_PAT            = 0X80, // PAGE ATTRIBUTE TABLE
    PTE_GLOBAL         = 0X100,
    PTE_FRAME          = 0X7FFFF000 // bits 12+
} PAGE_TABLE_FLAGS;
typedef enum {
    PDE_PRESENT        = 0x01, // bit 1
    PDE_READ_AND_WRITE = 0x02, // bit2
    PDE_USER           = 0x04, // bit 3
    PDE_WRITE_TROUGH   = 0X08, // bit 4 ...
    PDE_CACHE_DISABLE  = 0X10,
    PDE_ACCESSED       = 0X20,
    PDE_DIRTY          = 0X40,
    PDE_PAGE_SIZE      = 0X80,      // 0=4kKB 1=4MB
    PDE_GLOBAL         = 0X100,     // 4MB entry only
    PDE_PAT            = 0X2000,    // 4MB entry
    PDE_FRAME          = 0X7FFFF000 // bits 12+
} PAGE_DIR_FLAGS;

// Page table: handle 4MB each,1024 entries * 4096
typedef struct {
    pt_entry_t entries[PAGES_PER_TABLE];

} page_table_t;

// Page Directory: handle 4GB each , 1024 page tables +4MB
typedef struct {
    pd_entry_t entries[TABLES_PER_DIRECTORY];
} page_directory_t;


pt_entry_t *get_pt_entry(page_table_t *pt, virtual_address_t adress);

pd_entry_t *get_pd_entry(page_table_t *pd, virtual_address_t adress);

bool initialize_virtual_memory_manager(void);
void *get_physical_address(page_directory_t *dir, uint32_t virt);

void unmap_address(page_directory_t *dir, uint32_t virt);

void unmap_page_table(page_directory_t *dir, uint32_t virt);
bool create_page_table(page_directory_t *dir, uint32_t virt, uint32_t flags);

bool map_address(page_directory_t *dir, uint32_t phys, uint32_t virt,uint32_t flags);

int map_page(void *physical_address, void *virtual_address);

page_directory_t *get_page_directory();
bool set_page_directory(page_directory_t *pd);


