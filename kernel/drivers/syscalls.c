#include "syscalls.h"
#include "tables/idt/idt.h"
#include "tables/isr/isr.h"
#include "tables/irq/irq.h"
#include "../terminal/terminal.h"
#include "vga.h"
#include "../bootoptions.h"
#include <stdint.h>

void syscall_handler(registers_t *regs);
void reb() { reboot(); }

static void *syscalls[3] =
{
   &reb // Needs a char*
};
uint32_t num_syscalls = 3;

void initialise_syscalls()
{
   // Register our syscall handler.
   irq_install_handler (0x80, syscall_handler);
}

void syscall_handler(registers_t *regs)
{
   // Firstly, check if the requested syscall number is valid.
   // The syscall number is found in EAX.
   if (regs->eax >= num_syscalls)
       return;

   // Get the required syscall location.
   void *location = syscalls[regs->eax];

   // We don't know how many parameters the function wants, so we just
   // push them all onto the stack in the correct order. The function will
   // use all the parameters it wants, and we can pop them all back off afterwards.
   int ret;
   asm volatile (" \
     push %1; \
     push %2; \
     push %3; \
     push %4; \
     push %5; \
     call *%6; \
     pop %%ebx; \
     pop %%ebx; \
     pop %%ebx; \
     pop %%ebx; \
     pop %%ebx; \
   " : "=a" (ret) : "r" (regs->edi), "r" (regs->esi), "r" (regs->edx), "r" (regs->ecx), "r" (regs->ebx), "r" (location));
   regs->eax = ret;
} 