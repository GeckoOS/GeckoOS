#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <mem.h>
#include <mem/physical_mem.h>
#include <mem/paging.h>

static page_table_t *current_pml4 = NULL;

static page_table_t *alloc_table(void)
{
    page_table_t *t = (page_table_t *)allocate_blocks(1);
    if (t) memset(t, 0, sizeof(page_table_t));
    return t;
}

static pte_t *get_pte(page_table_t *pml4, uint64_t virt, bool create)
{
    /* PML4 */
    pte_t *pml4e = &pml4->entries[PML4_INDEX(virt)];
    if (!pte_is_present(*pml4e)) {
        if (!create) return NULL;
        page_table_t *pdpt = alloc_table();
        if (!pdpt) return NULL;
        *pml4e = (uint64_t)pdpt | PTE_PRESENT | PTE_WRITABLE;
    }

    /* PDPT */
    page_table_t *pdpt = (page_table_t *)pte_get_phys(*pml4e);
    pte_t *pdpte = &pdpt->entries[PDPT_INDEX(virt)];
    if (!pte_is_present(*pdpte)) {
        if (!create) return NULL;
        page_table_t *pd = alloc_table();
        if (!pd) return NULL;
        *pdpte = (uint64_t)pd | PTE_PRESENT | PTE_WRITABLE;
    }

    /* PD */
    page_table_t *pd = (page_table_t *)pte_get_phys(*pdpte);
    pte_t *pde = &pd->entries[PD_INDEX(virt)];
    if (!pte_is_present(*pde)) {
        if (!create) return NULL;
        page_table_t *pt = alloc_table();
        if (!pt) return NULL;
        *pde = (uint64_t)pt | PTE_PRESENT | PTE_WRITABLE;
    }

    /* PT */
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

    /* Identity-map 0 .. 4 MB (VGA, BIOS, early data) */
    for (uint64_t phys = 0; phys < 0x400000; phys += PAGE_SIZE) {
        if (!vmm_map(pml4, phys, phys, PTE_PRESENT | PTE_WRITABLE))
            return false;
    }

    /* Map kernel: physical 0x100000 → virtual 0xFFFFFFFF80100000 */
    uint64_t kernel_phys = 0x100000;
    uint64_t kernel_virt = 0xFFFFFFFF80100000ULL;
    for (uint64_t off = 0; off < 0x400000; off += PAGE_SIZE) {
        if (!vmm_map(pml4, kernel_phys + off, kernel_virt + off,
                     PTE_PRESENT | PTE_WRITABLE))
            return false;
    }

    return vmm_set_pml4(pml4);
}