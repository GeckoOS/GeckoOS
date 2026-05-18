; kernel/drivers/tables/irq/irq.s
bits 64

%macro IRQ 2
  [GLOBAL irq%1]
  irq%1:
    cli
    push qword 0            ; dummy error code
    push qword %2           ; interrupt vector number
    jmp irq_common_stub
%endmacro

IRQ   0,  32
IRQ   1,  33
IRQ   2,  34
IRQ   3,  35
IRQ   4,  36
IRQ   5,  37
IRQ   6,  38
IRQ   7,  39
IRQ   8,  40
IRQ   9,  41
IRQ  10,  42
IRQ  11,  43
IRQ  12,  44
IRQ  13,  45
IRQ  14,  46
IRQ  15,  47

[EXTERN irq_handler]

%macro pushaq 0
    push rax
    push rbx
    push rcx
    push rdx
    push rsi
    push rdi
    push rbp
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15
%endmacro

%macro popaq 0
    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rbp
    pop rdi
    pop rsi
    pop rdx
    pop rcx
    pop rbx
    pop rax
%endmacro

irq_common_stub:
    pushaq

    mov rdi, rsp
    call irq_handler

    popaq
    add rsp, 16             ; pop int_no and err_code
    sti
    iretq