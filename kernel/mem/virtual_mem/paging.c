
#include "paging.h"

#include <stdint.h>
#include <mem/mem.h>
#include <mem/physical_mem/physical_mem.h>

static page_directory_t *current_page_directory = 0;
static physical_address_t current_pd_adress     = 0;

// get entry in page table froma  given address
pt_entry_t *get_pt_entry(page_table_t *pt, virtual_address_t address)
{
    if (pt)
        return &pt->entries[PT_INDEX(address)];
    return 0;
}
// get entry in page directory froma  given address
pd_entry_t *get_pd_entry(page_table_t *pd, virtual_address_t address)
{
    if (pd)
        return &pd->entries[PT_INDEX(address)];
    return 0;
}
// Return a page (pointer to a page) for a given virtual adress in the current
// page directory
pt_entry_t *get_page(const virtual_address_t address)
{
    // Get page directory
    page_directory_t *pd = current_page_directory;

    // Get page table in directory
    pd_entry_t *entry   = &pd->entries[PD_INDEX(address)];
    page_table_t *table = (page_table_t *)PAGE_PHYS_ADRESS(entry);

    // Get page in table
    pt_entry_t *page = &table->entries[PT_INDEX(address)];

    // Return page
    return page;
}

void pt_add_flags(pt_entry_t *pt, uint32_t flags) { *pt |= flags; }

void pt_del_flags(pt_entry_t *pt, uint32_t flags) { *pt &= ~flags; }

void pd_add_flags(pd_entry_t *pd, uint32_t flags) { *pd |= flags; }

void pd_del_flags(pd_entry_t *pd, uint32_t flags) { *pd &= ~flags; }

//get the current page_directory
page_directory_t *get_page_directory() {
    return current_page_directory;
}

// Set the current page directory
bool set_page_directory(page_directory_t *pd)
{
    if (!pd) return false;

    current_page_directory = pd;

    // CR3 (Control register 3) holds address of the current page directory
    __asm__ __volatile__ ("movl %%EAX, %%CR3" : : "a"(current_page_directory) );

    return true;
}
//  Allocate page
void *allocate_page(pt_entry_t *page)
{

    void *block; //=allocate_block(1);
    if (block) {
        // MAP page to block
        SET_FRAME(page, (physical_address_t)block);
        SET_ATTRIBUTE(page, PTE_PRESENT);
    }
    return block;
}
// Fre a page of memory
void free_page(pt_entry_t *page)
{
    void *address = (void *)PAGE_PHYS_ADRESS(page);
    if (address)
        free_blocks(address, 1);

    CLEAR_ATTRIBUTE(page, PTE_PRESENT);
}

// Flush a single page from the TLB(traslation lookaside buffer)
void flush_tlb_entry(virtual_address_t address)
{
    __asm__ volatile("cli \n invlpg (%0) \n sti " ::"r"(address));
}
// Map a pag
int map_page(void *physical_address, void *virtual_address)
{
    page_directory_t *pd = current_page_directory;
    pd_entry_t *entry    = &pd->entries[PD_INDEX((uint32_t)virtual_address)];
    if ((*entry & PTE_PRESENT) != PTE_PRESENT) {
        // Page table not present? allocate it
        page_table_t *table = (page_table_t *)allocate_blocks(1);

        // download more ram xd
        if (!table)
            return false;

        // create page table
        memset(table, 0, sizeof(page_table_t));

        // create new entry
        pd_entry_t *entry = &pd->entries[PD_INDEX((uint32_t)virtual_address)];

        // Map in the table and enable attributes
        SET_ATTRIBUTE(entry, PDE_PRESENT);
        SET_ATTRIBUTE(entry, PDE_READ_AND_WRITE);
        SET_FRAME(entry, (physical_address_t)table);
    }

    // get_table
    page_table_t *table = (page_table_t *)PAGE_PHYS_ADRESS(entry);

    // Get page in the table
    pt_entry_t *page = &table->entries[PT_INDEX((uint32_t)virtual_address)];

    // Map in page
}

// Unmap a page
void unmap_page(void *virt_address)
{
    pt_entry_t *page = get_page((uint32_t)virt_address);

    SET_FRAME(page, 0); // Set physical address to 0 (effectively this is now a
                        // null pointer)
    CLEAR_ATTRIBUTE(page,
                    PTE_PRESENT); // Set as not present, will trigger a #PF
}

bool map_address(page_directory_t *dir, uint32_t phys, uint32_t virt,uint32_t flags)
{
    pt_entry_t *pd = dir->entries;
    if (pd[virt >> 22] == 0 && !create_page_table(dir, virt, flags))
        return false;

    ((uint32_t *)(pd[virt >> 22] & ~0xFFF))[virt << 10 >> 10 >> 12] =
        phys | flags;
    return true;
}

bool create_page_table(page_directory_t *dir, uint32_t virt, uint32_t flags)
{
    pt_entry_t *pd = dir->entries;
    if (pd[virt >> 22] == 0) {
        void *block = allocate_blocks(1);
        if (!block)
            return false;
        pd[virt >> 22] = (uint32_t)block | flags;
        memset((uint32_t *)pd[virt >> 22], 0, PAGE_SIZE);

        // Map page table into directory
        map_address(dir, (uint32_t)block, (uint32_t)block, flags);
    }
    return true;
}

void unmap_page_table(page_directory_t *dir, uint32_t virt)
{
    pt_entry_t *pd = dir->entries;
    if (pd[virt >> 22] != 0) {
        // Get mapped frame
        void *frame = (void *)(pd[virt >> 22] & 0x7FFFF000);

        // Unmap frame
        free_blocks(frame, 1);
        pd[virt >> 22] = 0;
    }
}

void unmap_address(page_directory_t *dir, uint32_t virt)
{
    pt_entry_t *pd = dir->entries;
    if (pd[virt >> 22] != 0)
        unmap_page_table(dir, virt);
}

void *get_physical_address(page_directory_t *dir, uint32_t virt)
{
    pt_entry_t *pd = dir->entries;
    if (pd[virt >> 22] == 0)
        return NULL;
    return (
        void *)((uint32_t *)(pd[virt >> 22] & ~0xFFF))[virt << 10 >> 10 >> 12];
}

// Initialize virtual memory manager
bool initialize_virtual_memory_manager(void)
{
    // Allocate page table for 0-4MB
    page_table_t *table = (page_table_t *)allocate_blocks(1);
    if (!table)
        return false; // Out of memory

    // Allocate a 3GB page table
    page_table_t *table3G = (page_table_t *)allocate_blocks(1);
    if (!table3G)
        return false; // Out of memory

    // Clear page tables
    memset(table, 0, sizeof(page_table_t));
    memset(table3G, 0, sizeof(page_table_t));

    // Identity map 1st 4MB of memory
    for (uint32_t i = 0, frame = 0x0, virt = 0x0; i < 1024;
         i++, frame += PAGE_SIZE, virt += PAGE_SIZE) {
        // Create new page
        pt_entry_t page = 0;
        SET_ATTRIBUTE(&page, PTE_PRESENT);
        SET_ATTRIBUTE(&page, PTE_READ_AND_WRITE);
        SET_FRAME(&page, frame);

        // Add page to 3GB page table
        table3G->entries[PT_INDEX(virt)] = page;
    }

    // Map kernel to 3GB+ addresses (higher half kernel)
    for (uint32_t i = 0, frame = KERNEL_ADDRESS, virt = 0xC0000000; i < 1024;
         i++, frame += PAGE_SIZE, virt += PAGE_SIZE) {
        // Create new page
        pt_entry_t page = 0;
        SET_ATTRIBUTE(&page, PTE_PRESENT);
        SET_FRAME(&page, frame);

        // Add page to 0-4MB page table
        table->entries[PT_INDEX(virt)] = page;
    }

    // Create a default page directory
    page_directory_t *dir = (page_directory_t *)allocate_blocks(3);
    if (!dir)
        return false; // Out of memory

    memset(dir, 0, sizeof(page_directory_t));

    // Map gfx info and font info for user processes
    pd_entry_t *ent = &dir->entries[PD_INDEX(0x0000C000)];
    pd_add_flags(ent, PDE_PRESENT | PDE_READ_AND_WRITE | PDE_USER);

    ent = &dir->entries[PD_INDEX(0x0000D000)];
    pd_add_flags(ent, PDE_PRESENT | PDE_READ_AND_WRITE | PDE_USER);

    table3G->entries[PT_INDEX(0x0000C000)] |= PTE_USER;
    table3G->entries[PT_INDEX(0x0000D000)] |= PTE_USER;

    // Map kernel table
    pd_entry_t *entry = &dir->entries[PD_INDEX(0xC0000000)];
    SET_ATTRIBUTE(entry, PDE_PRESENT);
    SET_ATTRIBUTE(entry, PDE_READ_AND_WRITE);
    SET_FRAME(entry,
              (physical_address_t)
                  table); // 3GB directory entry points to default page table

    // Map default table
    pd_entry_t *entry2 = &dir->entries[PD_INDEX(0x00000000)];
    SET_ATTRIBUTE(entry2, PDE_PRESENT);
    SET_ATTRIBUTE(entry2, PDE_READ_AND_WRITE);
    SET_FRAME(entry2,
              (physical_address_t)
                  table3G); // Default dir entry points to 3GB page table

    // Switch to page directory
    set_page_directory(dir);

    // Enable paging: Set PG (paging) bit 31 and PE (protection enable) bit 0 of
    // CR0
    __asm__ __volatile__(
        "movl %CR0, %EAX; orl $0x80000001, %EAX; movl %EAX, %CR0");

    return true;
}