#include <stdint.h>
#include "tss.h"
#include "../gdt/gdt.h"

tss_entry_t tss;

void tss_write(uint32_t idx, uint16_t ss0, uint32_t esp0) {
    // Base and limit of the TSS struct itself
    uint32_t base  = (uint32_t)&tss;
    uint32_t limit = base + sizeof(tss_entry_t);

    // Install a TSS descriptor into GDT slot `idx`
    gdt_set_gate(idx, base, limit, 0xE9, 0x00);
    // 0xE9 = Present | DPL=3 | 32-bit TSS (available)

    // Zero the TSS, then fill the fields we actually need
    uint8_t *p = (uint8_t *)&tss;
    for (uint32_t i = 0; i < sizeof(tss_entry_t); i++) p[i] = 0;

    tss.ss0  = ss0;   // kernel data segment selector (0x10)
    tss.esp0 = esp0;  // kernel stack pointer — call tss_set_kernel_stack() to update
    tss.cs   = 0x0b;  // ring-3 code  (0x08 | 0x03)
    tss.ss   = 0x13;  // ring-3 stack (0x10 | 0x03)
    tss.ds   = 0x13;
    tss.es   = 0x13;
    tss.fs   = 0x13;
    tss.gs   = 0x13;
}

// Call this every time the kernel stack changes (e.g. before entering user mode)
void tss_set_kernel_stack(uint32_t stack) {
    tss.esp0 = stack;
}