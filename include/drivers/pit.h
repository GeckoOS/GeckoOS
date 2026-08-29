#pragma once

#include <stdint.h>

void start_pit_timer(uint32_t frequency);
void pit_timer_wait_s(uint64_t ticks);
void pit_timer_wait_ms(uint32_t ms);

extern volatile uint64_t pit_timer;

#define PITHZ 100