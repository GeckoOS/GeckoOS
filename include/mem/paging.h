#pragma once
#include <stdint.h>
#include <stdbool.h>

#define PAGE_SIZE        4096ULL
#define PT_ENTRIES       512

#define PML4_INDEX(a)  (((uint64_t)(a) >> 39) & 0x1FF)
#define PDPT_INDEX(a)  (((uint64_t)(a) >> 30) & 0x1FF)
#define PD_INDEX(a)    (((uint64_t)(a) >> 21) & 0x1FF)
#define PT_INDEX(a)    (((uint64_t)(a) >> 12) & 0x1FF)

typedef enum {
    PTE_PRESENT        = (1ULL << 0),
    PTE_WRITABLE       = (1ULL << 1),
    PTE_USER           = (1ULL << 2),
    PTE_WRITE_THROUGH  = (1ULL << 3),
    PTE_CACHE_DISABLE  = (1ULL << 4),
    PTE_ACCESSED       = (1ULL << 5),
    PTE_DIRTY          = (1ULL << 6),
    PTE_HUGE           = (1ULL << 7),
    PTE_GLOBAL         = (1ULL << 8),
    PTE_NX             = (1ULL << 63),
    PTE_ADDR_MASK      = 0x000FFFFFFFFFF000ULL
} pte_flags_t;

typedef uint64_t pte_t;

typedef struct {
    pte_t entries[PT_ENTRIES];
} __attribute__((aligned(4096))) page_table_t;

static inline uint64_t pte_get_phys(pte_t e)   { return e & PTE_ADDR_MASK; }
static inline bool     pte_is_present(pte_t e) { return (e & PTE_PRESENT) != 0; }

bool          vmm_init(void);
bool          vmm_map(page_table_t *pml4, uint64_t phys, uint64_t virt, uint64_t flags);
void          vmm_unmap(page_table_t *pml4, uint64_t virt);
uint64_t      vmm_get_phys(page_table_t *pml4, uint64_t virt);
page_table_t *vmm_get_pml4(void);
bool          vmm_set_pml4(page_table_t *pml4);

uint64_t      mmio_map(uint64_t phys, uint64_t size);