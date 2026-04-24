#ifndef _IRQ_H
#define _IRQ_H

#include "../isr/isr.h"

extern void irq_install_handler(int irq, isr_t handler);
extern void irq_install();

#endif