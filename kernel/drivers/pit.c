#include "drivers/pit.h"
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

void start_pit_timer(uint32_t frequency) {
    irq_install_handler(0, timer_irq, 0);

    uint32_t divisor = 1193180 / frequency;

    outb(0x43, 0x36);

    uint8_t l = (uint8_t)(divisor & 0xFF);
    uint8_t h = (uint8_t)((divisor >> 8) & 0xFF );

    outb(0x40, l);
    outb(0x40, h);
} 

void pit_timer_wait_s(uint64_t s) {
    unsigned long eticks;

    eticks = pit_timer + (s * PITHZ);
    while(pit_timer < eticks) HALT();
}
void pit_timer_wait_ms(uint32_t ms) {
    unsigned long eticks;

    eticks = pit_timer + ms;
    while(pit_timer < eticks) HALT();
}