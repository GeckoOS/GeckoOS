#pragma once
#include "drivers/tables/isr.h"
#include <stdbool.h>
#include <stdint.h>

//The numbers, Mason what do they mean?
#define LAPIC_REG_ID        0x020 // Local APIC ID
#define LAPIC_REG_VERSION   0x030 // Local APIC Version
#define LAPIC_REG_TPR       0x080 // Task Priority Register
#define LAPIC_REG_EOI       0x0B0 // End of Interrupt
#define LAPIC_REG_SVR       0x0F0 // Spurious Interrupt Vector Register
#define LAPIC_REG_ISR_BASE  0x100 // In-Service Register (8 x 32-bit)
#define LAPIC_REG_IRR_BASE  0x200 // Interrupt Request Register (8 x 32-bit)
#define LAPIC_REG_ESR       0x280 // Error Status Register
#define LAPIC_REG_ICR_LOW   0x300 // Interrupt Command Register (low 32 bits)
#define LAPIC_REG_ICR_HIGH  0x310 // Interrupt Command Register (high 32 bits)
#define LAPIC_REG_TIMER_LVT 0x320 // Timer LVT Entry
#define LAPIC_REG_LINT0_LVT 0x350 // Local Interrupt 0 LVT
#define LAPIC_REG_LINT1_LVT 0x360 // Local Interrupt 1 LVT
#define LAPIC_REG_TIMER_ICR 0x380 // Timer Initial Count Register
#define LAPIC_REG_TIMER_CCR 0x390 // Timer Current Count Register
#define LAPIC_REG_TIMER_DCR 0x3E0 // Timer Divide Configuration Register

// SVR flags
#define LAPIC_SVR_ENABLE (1 << 8) // APIC Software Enable

// LVT flags
#define LAPIC_LVT_MASKED         (1 << 16) // Interrupt masked
#define LAPIC_LVT_TIMER_PERIODIC (1 << 17) // Periodic timer mode

#define lAPIC_ICR_LOW_DEST_OFFSET		18
#define LAPIC_IPI_LOW 0x000C4500;//low 32bits for initializing cores 

void cpu_set_apic_base(uint64_t apic);
uint64_t cpu_get_apic_base(void);

static inline uint64_t rdmsr(uint32_t msr);
static inline void wrmsr(uint32_t msr, uint64_t value);
// Checking which type of apic exists
bool cpu_has_apic();
bool cpu_has_x2apic();
volatile uint32_t *get_lapic_base();
// Initializing the local apic
int lapic_init();
void lapic_start_cores();

uint32_t lapic_get_id(void);
void lapic_write(uint32_t reg, uint32_t value);
uint32_t lapic_read(uint32_t reg);
void lapic_timer_start(void);
extern volatile uint64_t lapic_timer_tick;
void lapic_timer_handler(registers_t *regs);

void lapic_timer_wait(uint64_t ticks);