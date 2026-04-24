#ifndef VMM_H
#define VMM_H
#include <stdint.h>
// Paging stuff
// https://git.sr.ht/~queso_fuego/amateur_os_video_sources/tree/master/item/42_vmem_paging_malloc_free_high_half_kernel/include/memory/virtual_memory_manager.h
// https://codeberg.org/pizzuhh/AxiomOS/src/branch/main/src/kernel/include/memory/vmm.h#

#define PD_IDX(addr) ((addr) >> 22)
#define PT_IDX(addr) (((addr) >> 12) & 0x3FF)
#define PAGE_PHYS_ADDR(dir_entry) ((*dir_entry) & ~0xFFF)
#define SET_ATTRIBUTE(entry, attr) (*entry |= attr)
#define CLEAR_ATTRIBUTE(entry, attr) (*entry &= ~attr)
#define TEST_ATTRIBUTE(entry, attr) (*entry & attr)
#define SET_FRAME(entry, addr) (*entry = (*entry & ~0x7FFFF000) | addr)

#define PAGES_PER_TABLE 1024
#define TABLES_PER_DIRECTORY 1024
#define PAGE_SIZE 4096
#define KERNEL_ADDRESS 0x10000

typedef uint32_t pt_entry;
typedef uint32_t pd_entry;
typedef uint32_t physical_address;
typedef uint32_t virtual_address;

typedef enum {
    PTE_PRESENT         = 0x01,
    PTE_READ_WRITE      = 0x02,
    PTE_USER            = 0x04,
    PTE_WRITE_THROUGH   = 0x08,
    PTE_CACHE_DISABLED  = 0x10,
    PTE_ACCESSED        = 0x20,
    PTE_DIRTY           = 0x40,
    PTE_PAT             = 0x80,
    PTE_GLOBAL          = 0x100,
    PTE_FRAME           = 0x7FFFF000,
} PAGE_TABLE_FLAGS;

typedef enum {
    PDE_PRESENT         = 0x01,
    PDE_READ_WRITE      = 0x02,
    PDE_USER            = 0x04,
    PDE_WRITE_THROUGH   = 0x08,
    PDE_CACHE_DISABLED  = 0x10,
    PDE_ACCESSED        = 0x20,
    PDE_DIRTY           = 0x40,
    PDE_PAGE_SIZE       = 0x80,
    PDE_GLOBAL          = 0x100,
    PDE_PAT             = 0x2000,
    PDE_FRAME           = 0x7FFFF000,
} PAGE_DIR_FLAGS;

typedef struct {
    pt_entry entries[PAGES_PER_TABLE];
} page_table;

typedef struct {
    pd_entry entries[TABLES_PER_DIRECTORY];
} page_directory;

extern page_directory *current_page_directory;

pt_entry *get_pt_entry(page_table *pt, virtual_address addr);
pd_entry *get_pd_entry(page_directory *pd, virtual_address addr);
pd_entry *get_page(const virtual_address addr);
void *alloc_page(pt_entry *page);
void free_page(pt_entry *page);
int set_page_directory(page_directory *pd);
void flush_tlb_entry(virtual_address addr);
int map_page_old(virtual_address phys_address, physical_address virt_address);
void map_page(virtual_address vaddr, physical_address paddr, PAGE_TABLE_FLAGS flags);
void unmap_page(void *virtual_address);
int init_virtual_memory_manager(void);

#endif // VMM_H
