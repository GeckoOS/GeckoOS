; kernel/drivers/tables/irq/irq.s
bits 64
extern irq_handler

%macro IRQ 2
  [GLOBAL irq%1]
  irq%1:
    cli
    push qword 0            ; dummy error code
    push qword %2           ; interrupt vector number
    jmp irq_common_stub
%endmacro

section .data
;stub vectors   
%assign i 0
%rep 223
    IRQ i,i+32
%assign i i+1
%endrep

[GLOBAL irq_stub_table]
irq_stub_table:
%assign i 0
%rep 223
    dq irq %+ i        
%assign i i+1
%endrep


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
[GLOBAL irq_common_stub]
irq_common_stub:
    pushaq

    mov rdi, rsp
    call [irq_handler]

    popaq
    add rsp, 16             ; pop int_no and err_code
    sti
    iretq