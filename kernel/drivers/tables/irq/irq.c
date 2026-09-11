#include "drivers/acpi.h"
#include "drivers/apic/ioapic.h"
#include "drivers/apic/lapic.h"
#include <drivers/tables/idt.h>
#include <drivers/tables/isr.h>
#include <ports.h>
#include <stdint.h>
#include <terminal/printf.h>
#include <terminal/terminal.h>

extern void irq_common_stub();
#define MAX_IRQS 223

/*
    I think the IRQ is causing a general protection fault at boot, but it only happens sometimes
*/

extern void (*irq_stub_table[MAX_IRQS])(void);
//TODO:remeber this are numbered 32->0 and so on wen dealing with vectors
//!so the index means the irq not the idt number note for me
isr_t irq_routines_table[MAX_IRQS];

void irq_install_handler(int irq, isr_t handler, int flags) {
    for (int i = 0; i < isos_num; i++) {
        const struct madt_iso iso = isos[i];
        if (iso.irq == irq) {
            irq = iso.gsi;
            flags = iso.flags;
        }
    }

    irq_routines_table[irq] = handler;
    if (cpu_has_apic()) ioapic_redirect_irq(irq, irq + 32, flags);
}

void irq_uninstall_handler(int irq) { irq_routines_table[irq] = 0; }

void (*irq_handler)(registers_t*);

extern bool has_apic;
void apic_irq_handler(registers_t *regs) {
    int irq = regs->int_no - 32;
    if (irq_routines_table[irq])
        ((isr_t)(irq_routines_table[irq]))(regs);
    lapic_write(0xB0, 0);
}
void nonapic_irq_handler(registers_t *regs) {
    if (regs->int_no >= 40)
        outb(0xA0, 0x20);
    outb(0x20, 0x20);

    if (irq_routines_table[regs->int_no - 32] != 0) {
        isr_t handler = irq_routines_table[regs->int_no - 32];
        handler(regs);
    }
}

void irq_install() {
    for (int i = 32; i < 254; i++)
        idt_set_gate(i, (uint64_t)irq_stub_table[i-32], 0x08, 0x8E);
    if (has_apic) {
        irq_routines_table[16] = (isr_t)lapic_timer_handler;
        irq_handler = apic_irq_handler;
    } else
        irq_handler = nonapic_irq_handler;
}