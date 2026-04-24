#ifndef _TIMER_H
#define _TIMER_H

#include <stdint.h>

void timer_install();
void timer_wait(int ticks);
void timer_phase(int hz); /* Sets the timer hz */
int get_tick();

extern int actual_hz;

#define ticks_to_seconds(x) (actual_hz * x)
#define sleep(x) timer_wait(ticks_to_seconds(x))

#endif