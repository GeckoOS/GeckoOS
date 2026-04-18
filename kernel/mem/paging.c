#include "mem/paging.h"
#include <mem.h>
#include "ports.h"
#include "tables/isr/isr.h"
#include "tables/irq/irq.h"
#include <stdint.h>
#include <stdio.h>

// The book
uint32_t page_directory[1024] ALIGNED(4096);

// Like the first paper of the book
uint32_t first_page_table[1024] ALIGNED(4096);

// Inits page directory and its tables
void init_pages() {
    for (int i = 0; i < 1024; i++) { page_directory[i] = 0x00000002; }
    for (int i = 0; i < 1024; i++) { first_page_table[i] = (i * 0x1000) | 3; }

    // Putting the first paper into the book
    page_directory[0] = ((uint)first_page_table) | 3;

    // Putting the book into the shelf (Letting the processor know where is the first book through the register CR3)
    asm volatile(
        "mov %0, %%eax\n"
        "mov %%eax, %%cr3\n"
        :
        : "r"(page_directory)
    ); // You can see if the cr3 changed looking at the registers, or printing the address of the book
    
    // Now someone more uses the book that you putted in the shelf (The processor using those virtual addresses through the register CR0)
    __asm__ __volatile__ (
        "invlpg (%0)"
        : : "r"(first_page_table)
    );
    __asm__ __volatile__(
        "mov %CR0, %EAX\n"
        "orl $0x80000001, %EAX\n"
        "mov %EAX, %CR0"
    ); // Please don't crash
}