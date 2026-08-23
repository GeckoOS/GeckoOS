#include <drivers/tables/isr.h>
#include <stdint.h>
#include <terminal/printf.h>

isr_t interrupt_handlers[256];

extern void kmain();
void ud_exception_handler(registers_t* regs) {
    printf("Exception UD\nInvalid Opcode: 0x%x in 0x%p\nJumping back to main function...\n", *(uint8_t*)regs->rip, regs->rip);
    asm volatile(
        "mov %0, %%rax\n"
        "jmpq *%%rax"
        : : "r"((uint64_t)kmain) // Everything will be slow for some reason
    );
    for(;;)asm volatile("hlt");
}

void register_interrupt_handler(uint8_t n, isr_t handler) {
    interrupt_handlers[n] = handler;
} 

const char* interrupt_string_table[] = {
    "Divide Error",
    "Debug Exception",
    "NMI Interrupt",
    "Breakpoint",
    "Overflow",
    "BOUND Range Exceeded",
    "Invalid Opcode (Undefined Opcode)",
    "Device Not Available (No Math Coprocessor)",
    "Double Fault",
    "Coprocessor Segment Overrun (reserved)",
    "Invalid TSS",
    "Segment Not Present",
    "Stack-Segment Fault",
    "General Protection",
    "Page Fault",
    "Intel reserved",
    "x87 FPU Floating-Point Error (Math Fault)",
    "Alignment Check",
    "Machine Check",
    "SIMD Floating-Point Exception",
    "Virtualization Exception",
    "Control Protection Exception"
};

void isr_handler(registers_t *regs) {
    isr_t handler = interrupt_handlers[regs->int_no];
    
    if (handler) {
        handler(regs);
    } else {
        char check = regs->int_no > 21;
        printf("The interruption 0x%x dosen't has a designed handler!%s%s\n", regs->int_no,
            check > 21 ? "" : " But that interruption has an error string: ",
            check > 21 ? "" : interrupt_string_table[regs->int_no]);     
    }
    // for (;;);
}