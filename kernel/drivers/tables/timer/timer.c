// Pumpkicks

#include <terminal/terminal.h> // for printf
#include <drivers/tables/isr.h>
#include <colors.h>
#include <drivers/tables/irq.h>

int timer_ticks = 0;
int actual_hz = 50; // Set to 50 as the default

void timer_handler(registers_t* r)
{
    /* Increment our 'tick count' */
    timer_ticks++;

/*     Every 18 clocks (approximately 1 second), we will
       display a message on the screen
    if (timer_ticks % 18 == 0)
    {
        print("One second has passed\n");
    } */
}
void timer_phase(int hz)
{
    actual_hz = hz;
    int divisor = 1193180 / hz;       /* Calculate our divisor */
    outb(0x43, 0x36);             /* Set our command byte 0x36 */
    outb(0x40, divisor & 0xFF);   /* Set low byte of divisor */
    outb(0x40, divisor >> 8);     /* Set high byte of divisor */
}

/* Sets up the system clock by installing the timer handler
*  into IRQ0 */
void timer_install()
{
    /* Installs 'timer_handler' to IRQ0 */
    irq_install_handler(0, timer_handler);
}

// FINALLY, the cpu was returning an int 6, but i set some variables to a pointer, and then it began working!
void timer_wait(int ticks)
{
    unsigned long eticks;

    eticks = timer_ticks + ticks;
    while(timer_ticks < eticks) { PAUSE(); }
}
int get_tick() { return timer_ticks; }