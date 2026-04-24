#pragma once

#include "terminal/terminal.h"
#include <drivers/tables/isr/isr.h>
#include <stdint.h>

static inline void page_fault(registers_t* regs)
{
	// A page fault has occured.
	// The faulting address is stored in the CR2 register.
	uint32_t faulting_address;
	asm volatile("mov %%cr2, %0" : "=r" (faulting_address));

	// The error code gives us detailing og what happened.
	int present	= !(regs->err_code & 0x1);	// Page not present
	int rw		= regs->err_code & 0x2;		// Write operation?
	int us		= regs->err_code & 0x4;		// Processor was in user-mode?
	int reserved= regs->err_code & 0x8;		// Overwritten CPU-reserved bits of page entry?
	int id		= regs->err_code & 0x10;		// Caused by an instruction fetch?

	// Output en error message.
	print("Page fault! ( ");
	if (present)
		print("present ");
	if (rw)
		print("read-only ");
	if (us)
		print("user-mode ");
	if (reserved)
		print("reserved ");

	print(") at ");
	print_hex(faulting_address);
	print("\n");
	while (1);
	// PANIC("Page fault");
}
