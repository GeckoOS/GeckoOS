void switch_to_user_mode()
{
    // Set up a stack structure for switching to user mode.
    asm volatile("  \
        cli; \
        mov $0x23, %ax; \
        mov %ax, %ds; \
        mov %ax, %es; \
        mov %ax, %fs; \
        mov %ax, %gs; \
                    \
        mov %esp, %eax; \
        pushl $0x23; \
        pushl %eax; \
        pushf; \
        pushl $0x1B; \
        push $1f; \
        iret; \
    1: \
        ");
    asm volatile( // Allowing STI in usermode (Without this asm slice, STI will cause a general protection fault)
        "pop %eax\n"
        "orl $0x200, %eax\n"
        "push %eax"
    );
    asm volatile("sti");
}