; defines for our stack, and for a return value from the multiboot
; bootloader
STACK_SIZE equ 16384
MULTIBOOT_OUTPUT_MAGIC equ 0x2BADB002
HIGHER equ 0xC0000000 ; Unused

; define the "kernel_main" function, which this code is going to call
extern kernel_main
extern _kernel_start
extern _kernel_end

section .bootstrap_stack
    align 4096
stack_bottom:
    resb 16384 ; 16 KiB
stack_top:

section .text

hang:
.hlt_loop:
  hlt
  jmp .hlt_loop

global _entry
_entry:
    cmp eax, MULTIBOOT_OUTPUT_MAGIC
    jne hang

    mov esp, stack_top
    push ebx
    call kernel_main

    mov al, 0xfe ; If err (the kernel returns) reboots (maybe go into a bootloop)
    out 0x64, al
