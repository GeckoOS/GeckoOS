#ifndef _ISR_H
#define _ISR_H

#include <stdint.h>

typedef struct registers
{
    /* Saved by pushaq macro (isr.s) */
    uint64_t r15, r14, r13, r12, r11, r10, r9, r8;
    uint64_t rbp, rdi, rsi, rdx, rcx, rbx, rax;

    /* Pushed by stub */
    uint64_t int_no;
    uint64_t err_code;

    /* Pushed by the CPU automatically */
    uint64_t rip;
    uint64_t cs;
    uint64_t rflags;
    uint64_t rsp;
    uint64_t ss;
} registers_t;

typedef void (*isr_t)(registers_t*);

#endif