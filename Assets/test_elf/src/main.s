section .text
global _start
_start:
    xor eax, eax
    mov ecx, 10
loop:
    add eax, ecx
    dec ecx
    jnz loop
    ret