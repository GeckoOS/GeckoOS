#include <stdint.h>
#include "gdt.h"
#include "../tss/tss.h"

gdt_entry_t gdt_entries[6]; //ember2819: 6 slots: null, k-code, k-data, u-code, u-data, TSS
gdt_ptr_t   gdt_ptr;

void init_gdt()
{
   gdt_ptr.limit = (sizeof(gdt_entry_t) * 6) - 1; //ember2819: was 5, now 6 to include TSS
   gdt_ptr.base  = (uint32_t)&gdt_entries;

   gdt_set_gate(0, 0, 0, 0, 0);                // Null segment
   gdt_set_gate(1, 0, 0xFFFFFFFF, 0x9A, 0xCF); // Kernel code  (ring 0)
   gdt_set_gate(2, 0, 0xFFFFFFFF, 0x92, 0xCF); // Kernel data  (ring 0)
   gdt_set_gate(3, 0, 0xFFFFFFFF, 0xFA, 0xCF); // User code    (ring 3)
   gdt_set_gate(4, 0, 0xFFFFFFFF, 0xF2, 0xCF); // User data    (ring 3)
   // Slot 5 is filled by tss_write() below

   gdt_flush((uint32_t)&gdt_ptr);

   tss_write(5, 0x10, 0);
   extern void tss_flush();
   tss_flush();
}

// Set the value of one GDT entry.
void gdt_set_gate(int32_t num, uint32_t base, uint32_t limit, uint8_t access, uint8_t gran)
{
   gdt_entries[num].base_low    = (base & 0xFFFF);
   gdt_entries[num].base_middle = (base >> 16) & 0xFF;
   gdt_entries[num].base_high   = (base >> 24) & 0xFF;

   gdt_entries[num].limit_low   = (limit & 0xFFFF);
   gdt_entries[num].granularity = (limit >> 16) & 0x0F;

   gdt_entries[num].granularity |= gran & 0xF0;
   gdt_entries[num].access      = access;
}