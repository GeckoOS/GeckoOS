#pragma once

#include <stdint.h>

void start_pit_timer(uint32_t frequency);
void pit_timer_wait(uint64_t ticks);

extern volatile uint64_t pit_timer;

#define PITHZ 100