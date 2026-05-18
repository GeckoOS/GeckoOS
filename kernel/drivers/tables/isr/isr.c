#include <drivers/tables/isr.h>
#include <terminal/terminal.h>

void isr_handler(registers_t *regs)
{
    print("received interrupt: ");
    print_hex((uint32_t)regs->int_no);
    print("\n");
}