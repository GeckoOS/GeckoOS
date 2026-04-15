#include "isr.h"
#include "../../../terminal/terminal.h"
#include "../../panic.h"

void isr_handler(registers_t regs)
{
    print("recieved interrupt: ");
    print_hex(regs.int_no); print("\n");
    if (regs.int_no == 0xd) {
        panic("General protect fault\n");
    }
    print("\n");
} 