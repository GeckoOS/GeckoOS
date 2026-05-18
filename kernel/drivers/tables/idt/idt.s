bits 64
[GLOBAL idt_flush]

idt_flush:
    ; RDI contains the pointer to idt_ptr_t (System V AMD64 calling convention)
    lidt [rdi]
    ret