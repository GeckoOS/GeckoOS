#include <mem/vmm.h>
#include <pagefault.h>
#include <mem/pmm.h>
#include <drivers/tables/irq/irq.h>
#include <stdint.h>
#include <string.h>

// https://codeberg.org/pizzuhh/AxiomOS/src/branch/main/src/kernel/include/memory/vmm.c

page_directory *current_page_directory = 0;

pt_entry *get_pt_entry(page_table *pt, virtual_address addr) {
    if (pt) return &pt->entries[PT_IDX(addr)];
    return 0;
}

pd_entry *get_pd_entry(page_directory *pd, virtual_address addr) {
    if (pd) return &pd->entries[PT_IDX(addr)];
    return 0;
}

pd_entry *get_page(const virtual_address addr) {
    page_directory *pd = current_page_directory;

    pd_entry *entry = &pd->entries[PD_IDX(addr)];
    page_table *table = (page_table*)PAGE_PHYS_ADDR(entry);

    pt_entry *page = &table->entries[PT_IDX(addr)];

    return page;
}

void *alloc_page(pt_entry *page) {
    void *block = pmm_alloc_blocks(1);
    if (block) {
        SET_FRAME(page, (physical_address)block);
        SET_ATTRIBUTE(page, PTE_PRESENT);
    }
    return block;
}

void free_page(pt_entry *page) {
    void *addr = (void*)PAGE_PHYS_ADDR(page);
    if (addr) pmm_free_blocks(addr, 1);
    CLEAR_ATTRIBUTE(page, PTE_PRESENT);
}

int set_page_directory(page_directory *pd) {
    if (!pd) return 0;

    current_page_directory = pd;

    asm volatile ("movl %%EAX, %%CR3" : : "a"(current_page_directory));

    return 1;
}

void flush_tlb_entry(virtual_address addr) {
    asm volatile ("cli; invlpg (%0); sti;" : : "r"(addr));
}

void map_page(virtual_address vaddr, physical_address paddr, PAGE_TABLE_FLAGS flags) {
    uint32_t pd_idx = PD_IDX(vaddr);
    uint32_t pt_idx = PT_IDX(vaddr);

    pd_entry *pde = &current_page_directory->entries[pd_idx];

    page_table *pt;

    if (!TEST_ATTRIBUTE(pde, PDE_PRESENT)) {
        pt = (page_table*)pmm_alloc_blocks(1);

        memset(pt, 0, sizeof(page_table));

        SET_FRAME(pde, (physical_address)pt);
        SET_ATTRIBUTE(pde, PDE_PRESENT | PDE_READ_WRITE);
    } else {
        pt = (page_table*)(PAGE_PHYS_ADDR(pde));
    }

    pt_entry *pte = &pt->entries[pt_idx];

    SET_FRAME(pte, paddr);
    SET_ATTRIBUTE(pte, flags);
}

// i am blind i am blind i am blind i am blind i am blind
int map_page_old(virtual_address virt_address, physical_address phys_address) {
    page_directory *pd = current_page_directory;

    pd_entry *pdentry = &pd->entries[PD_IDX(virt_address)];

    if (!TEST_ATTRIBUTE(pdentry, PDE_PRESENT)) {
        page_table *pt = (page_table*)pmm_alloc_blocks(1);
        if (!pt) return 0;
        memset(pt, 0, sizeof(page_table));

        SET_FRAME(pdentry, (physical_address)pt);
        SET_ATTRIBUTE(pdentry, PDE_PRESENT | PDE_READ_WRITE);
    }
    page_table *table = (page_table *)PAGE_PHYS_ADDR(pdentry);

    pt_entry *pte = &table->entries[PT_IDX(virt_address)];

    SET_FRAME(pte, phys_address);
    SET_ATTRIBUTE(pte, PTE_PRESENT | PTE_READ_WRITE);

    return 1;
}

void unmap_page(void *virtual_address) {
    pt_entry *page = get_page((uint32_t)virtual_address);

    SET_FRAME(page, 0);
    CLEAR_ATTRIBUTE(page, PTE_PRESENT);
}

int init_virtual_memory_manager(void) {
    // Create a default page directory
    page_directory *dir = (page_directory *)pmm_alloc_blocks(3);

    if (!dir) for (;;); // Out of memory

    // Clear page directory and set as current
    memset(dir, 0, sizeof(page_directory));
    for (uint32_t i = 0; i < 1024; i++)
        dir->entries[i] = 0x02; // Supervisor, read/write, not present

        // Allocate page table for 0-4MB
    page_table *table = (page_table *)pmm_alloc_blocks(1);

    if (!table) return 0;   // Out of memory

    // Allocate a 3GB page table
    page_table *table3G = (page_table *)pmm_alloc_blocks(1);

    if (!table3G) return 0;   // Out of memory

    // Clear page tables
    memset(table, 0, sizeof(page_table));
    memset(table3G, 0, sizeof(page_table));

    // Identity map 1st 4MB of memory
    for (uint32_t i = 0, frame = 0x0, virt = 0x0; i < 1024; i++, frame += PAGE_SIZE, virt += PAGE_SIZE) {
        // Create new page
        pt_entry page = 0;
        SET_ATTRIBUTE(&page, PTE_PRESENT);
        SET_ATTRIBUTE(&page, PTE_READ_WRITE);
        SET_FRAME(&page, frame);

        // Add page to 3GB page table
        table3G->entries[PT_IDX(virt)] = page;
    }

    // Map kernel to 3GB+ addresses (higher half kernel)
    for (uint32_t i = 0, frame = KERNEL_ADDRESS, virt = 0xC0000000; i < 1024; i++, frame += PAGE_SIZE, virt += PAGE_SIZE) {
        // Create new page
        pt_entry page = 0;
        SET_ATTRIBUTE(&page, PTE_PRESENT);
        SET_ATTRIBUTE(&page, PTE_READ_WRITE);
        SET_FRAME(&page, frame);

        // Add page to 0-4MB page table
        table->entries[PT_IDX(virt)] = page;
    }

    pd_entry *entry = &dir->entries[PD_IDX(0xC0000000)];
    SET_ATTRIBUTE(entry, PDE_PRESENT);
    SET_ATTRIBUTE(entry, PDE_READ_WRITE);
    SET_FRAME(entry, (physical_address)table); // 3GB directory entry points to default page table

    pd_entry *entry2 = &dir->entries[PD_IDX(0x00000000)];
    SET_ATTRIBUTE(entry2, PDE_PRESENT);
    SET_ATTRIBUTE(entry2, PDE_READ_WRITE);
    SET_FRAME(entry2, (physical_address)table3G);    // Default dir entry points to 3GB page table

    // irq_install_handler(14, page_fault);
    // Switch to page directory
    set_page_directory(dir);
    // Enable paging: Set PG (paging) bit 31 and PE (protection enable) bit 0 of CR0
    __asm__ __volatile__ ("movl %CR0, %EAX; orl $0x80010000, %EAX; movl %EAX, %CR0");

    return 1;
}
