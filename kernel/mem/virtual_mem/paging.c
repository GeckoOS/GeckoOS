#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <mem.h>
#include <mem/physical_mem.h>
#include <mem/paging.h>

static page_table_t *current_pml4 = NULL;

#define MMIO_VIRT_BASE 0xFFFFFFFF90000000ULL
static uint64_t mmio_virt_next = MMIO_VIRT_BASE;

static page_table_t *alloc_table(void)
{
    page_table_t *t = (page_table_t *)allocate_blocks(1);
    if (t) memset(t, 0, sizeof(page_table_t));
    return t;
}

static pte_t *get_pte(page_table_t *pml4, uint64_t virt, bool create)
{
    pte_t *pml4e = &pml4->entries[PML4_INDEX(virt)];
    if (!pte_is_present(*pml4e)) {
        if (!create) return NULL;
        page_table_t *pdpt = alloc_table();
        if (!pdpt) return NULL;
        *pml4e = (uint64_t)pdpt | PTE_PRESENT | PTE_WRITABLE;
    }

    page_table_t *pdpt = (page_table_t *)pte_get_phys(*pml4e);
    pte_t *pdpte = &pdpt->entries[PDPT_INDEX(virt)];
    if (!pte_is_present(*pdpte)) {
        if (!create) return NULL;
        page_table_t *pd = alloc_table();
        if (!pd) return NULL;
        *pdpte = (uint64_t)pd | PTE_PRESENT | PTE_WRITABLE;
    }

    page_table_t *pd = (page_table_t *)pte_get_phys(*pdpte);
    pte_t *pde = &pd->entries[PD_INDEX(virt)];
    if (!pte_is_present(*pde)) {
        if (!create) return NULL;
        page_table_t *pt = alloc_table();
        if (!pt) return NULL;
        *pde = (uint64_t)pt | PTE_PRESENT | PTE_WRITABLE;
    }

    page_table_t *pt = (page_table_t *)pte_get_phys(*pde);
    return &pt->entries[PT_INDEX(virt)];
}

static void flush_tlb(uint64_t virt)
{
    __asm__ volatile("invlpg (%0)" : : "r"(virt) : "memory");
}

page_table_t *vmm_get_pml4(void)
{
    return current_pml4;
}

bool vmm_set_pml4(page_table_t *pml4)
{
    if (!pml4) return false;
    current_pml4 = pml4;
    __asm__ volatile("mov %0, %%cr3" : : "r"((uint64_t)pml4) : "memory");
    return true;
}

bool vmm_map(page_table_t *pml4, uint64_t phys, uint64_t virt, uint64_t flags)
{
    pte_t *pte = get_pte(pml4, virt, true);
    if (!pte) return false;
    *pte = (phys & PTE_ADDR_MASK) | (flags & ~PTE_ADDR_MASK) | PTE_PRESENT;
    flush_tlb(virt);
    return true;
}

void vmm_unmap(page_table_t *pml4, uint64_t virt)
{
    pte_t *pte = get_pte(pml4, virt, false);
    if (!pte) return;
    *pte = 0;
    flush_tlb(virt);
}

uint64_t vmm_get_phys(page_table_t *pml4, uint64_t virt)
{
    pte_t *pte = get_pte(pml4, virt, false);
    if (!pte || !pte_is_present(*pte)) return 0;
    return pte_get_phys(*pte);
}

bool vmm_init(void)
{
    page_table_t *pml4 = alloc_table();
    if (!pml4) return false;

    /*
     * Identity-map the first 16 MB. The physical allocator lives at
     * 0x500000 (5 MB) and the page tables it hands out need to be
     * reachable after CR3 switches. 4 MB wasn't enough.
     */
    for (uint64_t phys = 0; phys < 0x1000000; phys += PAGE_SIZE) {
        if (!vmm_map(pml4, phys, phys, PTE_PRESENT | PTE_WRITABLE))
            return false;
    }

    uint64_t kernel_phys = 0x100000;
    uint64_t kernel_virt = 0xFFFFFFFF80100000ULL;
    for (uint64_t off = 0; off < 0x400000; off += PAGE_SIZE) {
        if (!vmm_map(pml4, kernel_phys + off, kernel_virt + off,
                     PTE_PRESENT | PTE_WRITABLE))
            return false;
    }

    return vmm_set_pml4(pml4);
}

uint64_t mmio_map(uint64_t phys, uint64_t size)
{
    page_table_t *pml4 = vmm_get_pml4();
    if (!pml4) return 0;

    uint64_t phys_aligned = phys & ~(PAGE_SIZE - 1);
    uint64_t offset       = phys - phys_aligned;
    uint64_t pages        = (size + offset + PAGE_SIZE - 1) / PAGE_SIZE;

    uint64_t virt_base = mmio_virt_next;
    mmio_virt_next += pages * PAGE_SIZE;

    for (uint64_t i = 0; i < pages; i++) {
        if (!vmm_map(pml4,
                     phys_aligned + i * PAGE_SIZE,
                     virt_base    + i * PAGE_SIZE,
                     PTE_PRESENT | PTE_WRITABLE | PTE_CACHE_DISABLE))
            return 0;
    }

    return virt_base + offset;
}