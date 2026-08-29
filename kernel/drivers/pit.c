#include "ports.h"
#include <drivers/tables/irq.h>
#include <stdint.h>
#include <terminal/printf.h>
#include <drivers/acpi.h>

volatile uint64_t pit_timer = 0;

void pit_timer_wait_s(uint64_t ticks);

static void timer_irq(registers_t* _) {
    pit_timer++;
}

extern struct madt_iso pit_timer_iso;
void start_pit_timer(uint32_t frequency) {
    irq_install_handler(pit_timer_iso.gsi, timer_irq, pit_timer_iso.flags);

    uint32_t divisor = 1193180 / frequency;

    outb(0x43, 0x36);

    uint8_t l = (uint8_t)(divisor & 0xFF);
    uint8_t h = (uint8_t)((divisor >> 8) & 0xFF );

    outb(0x40, l);
    outb(0x40, h);
} 

void pit_timer_wait_s(uint64_t ticks) {
    unsigned long eticks;

    eticks = pit_timer + ticks;
    while(pit_timer < eticks) HALT();
}
void pit_timer_wait_ms(uint32_t ms) {
    uint32_t ticks = (ms + 9) / 10;

    uint32_t start = pit_timer;
    while ((pit_timer - start) < ticks) HALT();
}