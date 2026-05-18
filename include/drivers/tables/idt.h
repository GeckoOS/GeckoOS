#ifndef _IDT_H
#define _IDT_H

#include <stdint.h>
#include <drivers/tables/isr.h>

struct idt_entry_struct
{
    uint16_t base_lo;
    uint16_t sel;
    uint8_t  ist;       /* Interrupt Stack Table index (0 = legacy stack) */
    uint8_t  flags;
    uint16_t base_mid;
    uint32_t base_hi;
    uint32_t zero;      /* reserved */
} __attribute__((packed));
typedef struct idt_entry_struct idt_entry_t;

struct idt_ptr_struct
{
    uint16_t limit;
    uint64_t base;      /* 64-bit base address */
} __attribute__((packed));
typedef struct idt_ptr_struct idt_ptr_t;

/* ISR stubs declared in isr.s */
extern void isr0 ();
extern void isr1 ();
extern void isr2 ();
extern void isr3 ();
extern void isr4 ();
extern void isr5 ();
extern void isr6 ();
extern void isr7 ();
extern void isr8 ();
extern void isr9 ();
extern void isr10();
extern void isr11();
extern void isr12();
extern void isr13();
extern void isr14();
extern void isr15();
extern void isr16();
extern void isr17();
extern void isr18();
extern void isr19();
extern void isr20();
extern void isr21();
extern void isr22();
extern void isr23();
extern void isr24();
extern void isr25();
extern void isr26();
extern void isr27();
extern void isr28();
extern void isr29();
extern void isr30();
extern void isr31();

extern void idt_flush(uint64_t);
void init_idt();
void idt_set_gate(uint8_t num, uint64_t base, uint16_t sel, uint8_t flags);

#endif