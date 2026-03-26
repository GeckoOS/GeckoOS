#include "isr.h"
#include "../../../terminal/terminal.h"

void isr_handler(registers_t regs)
{
    print("recieved interrupt: ");
    print_hex(regs.int_no);
    print("\n");
} 