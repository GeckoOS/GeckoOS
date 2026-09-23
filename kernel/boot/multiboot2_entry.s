bits 32

MULTIBOOT2_MAGIC    equ 0xE85250D6
MULTIBOOT2_ARCH     equ 0
HEADER_LENGTH       equ (mb2_header_end - mb2_header_start)
HEADER_CHECKSUM     equ -(MULTIBOOT2_MAGIC + MULTIBOOT2_ARCH + HEADER_LENGTH)

section .multiboot2_header
align 8
mb2_header_start:
    dd MULTIBOOT2_MAGIC
    dd MULTIBOOT2_ARCH
    dd HEADER_LENGTH
    dd HEADER_CHECKSUM
align 8
    ; Framebuffer request tag
    dw 5                          ; type = MULTIBOOT2_HEADER_TAG_FRAMEBUFFER
    dw 0                          ; flags = 0 (optional)
    dd 20                         ; size
    dd 1024                       ; width
    dd 768                        ; height
    dd 32                         ; depth (bpp)
align 8
    ; End tag
    dw 0                          ; type = MULTIBOOT2_HEADER_TAG_END
    dw 0                          ; flags
    dd 8                          ; size
mb2_header_end:

section .bootstrap_data nobits alloc write
align 4096
pml4_table:     resb 4096
pdpt_table_lo:  resb 4096
pdpt_table_hi:  resb 4096
pd_table:       resb 4096
pd_table_fb:    resb 4096

align 16
early_stack_bottom: resb 16384
early_stack_top:

section .bss
align 16
kernel_stack_bottom:
    resb 65536
kernel_stack_top:

section .data
align 8
gdt64_start:
    dq 0                    ; null descriptor
gdt64_code:
    dq 0x00AF9A000000FFFF   ; 64-bit code: L=1, P=1, DPL=0
gdt64_data:
    dq 0x00CF92000000FFFF   ; 64-bit data: P=1, DPL=0
gdt64_user_data:
    dq 0x00CF92000000FFFF
gdt64_user_code:
    dq 0x3FFFC000007D7D00
gdt64_tss:
    dq 0
    dq 0
gdt64_end:

gdt64_ptr:
    dw gdt64_end - gdt64_start - 1
    dq gdt64_start          ; 64-bit base

magic:
    dd 0
mbi:
    dd 0

section .text.multiboot2_entry
global multiboot2_entry
global gdt64_tss
extern multiboot2_main
extern global_tss

multiboot2_entry:
    mov ebp, eax

    mov esp, early_stack_top

    mov [magic], eax
    mov [mbi], ebx

    mov edi, pml4_table ; This zeroes all the tables
    mov ecx, 5120
    xor eax, eax
    rep stosd

;    mov ebx, pml4_table ; This block of code is USELESS
;    mov ecx, (4 * 4096) / 4
;    xor eax, eax
;.zero_tables:
;    mov [ebx], eax
;    add ebx, 4
;    loop .zero_tables

    mov eax, pdpt_table_lo
    or  eax, 0x3
    mov [pml4_table], eax

    mov eax, pdpt_table_hi
    or  eax, 0x3
    mov dword [pml4_table + 511*8], eax
    mov dword [pml4_table + 511*8 + 4], 0

    ; PDPT_LO[0] -> pd_table
    mov eax, pd_table
    or  eax, 0x3
    mov [pdpt_table_lo], eax

    ; PDPT_HI[510] --> pd_table 
    mov eax, pd_table
    or  eax, 0x3
    mov dword [pdpt_table_hi + 510*8], eax
    mov dword [pdpt_table_hi + 510*8 + 4], 0

    ; PD: 512 × 2 MB huge pages covering 0 .. 1 GB
    mov ecx, 0
.fill_pd:
    mov eax, 0x200000
    mul ecx                 ; eax = ecx * 2MB
    or  eax, (1 << 7) | 0x3  ; huge + present + writable
    mov [pd_table + ecx*8],     eax
    mov dword [pd_table + ecx*8 + 4], 0
    inc ecx
    cmp ecx, 512
    jne .fill_pd

    ; PD_FB: 512 × 2 MB huge pages covering 3 .. 4 GB (for framebuffer)
    mov ecx, 0
.fill_pd_fb:
    mov eax, 0x200000
    mul ecx                 ; eax = ecx * 2MB
    add eax, 0xC0000000     ; physical address = 3GB + ecx*2MB
    or  eax, (1 << 7) | 0x3  ; huge + present + writable
    mov [pd_table_fb + ecx*8], eax
    mov dword [pd_table_fb + ecx*8 + 4], 0
    inc ecx
    cmp ecx, 512
    jne .fill_pd_fb

    ; PDPT_LO[3] -> pd_table_fb  (maps 3-4 GB identity)
    mov eax, pd_table_fb
    or  eax, 0x3
    mov [pdpt_table_lo + 3*8], eax

    ; enable PAE
    mov eax, cr4
    or  eax, (1 << 5)
    mov cr4, eax

    mov eax, pml4_table
    mov cr3, eax

    mov ecx, 0xC0000080
    rdmsr
    or  eax, (1 << 8)
    wrmsr

    ; enable paging
    mov eax, cr0
    or  eax, 0x80000001
    mov cr0, eax

    lgdt [gdt64_ptr]

    jmp 0x08:long_mode_entry

bits 64
long_mode_entry:
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    mov rsp, kernel_stack_top

    ; Setting the tss entry in the gdt, asked chatgpt some questions about this (I'm not that good with pure ASM)

    ; Putting the limit
    mov rax, 103
    mov byte [gdt64_tss], al
    shr rax, 8
    mov byte [gdt64_tss + 1], al
    mov rax, 103
    shr rax, 16
    mov byte [gdt64_tss + 6], al

    ; Putting the base
    mov rax, global_tss
    mov byte [gdt64_tss + 2], al
    shr eax, 8
    mov byte [gdt64_tss + 3], al
    mov rax, global_tss
    shr eax, 16
    mov byte [gdt64_tss + 4], al
    mov rax, global_tss
    shr eax, 24
    mov byte [gdt64_tss + 7], al
    mov rax, global_tss
    shr rax, 32
    mov dword [gdt64_tss + 8], eax
    mov dword [gdt64_tss + 12], 0

    push rdi
    mov edi, gdt64_tss + 8
    mov ecx, 2
    xor eax, eax
    rep stosd
    pop rdi

    ; Putting access byte
    mov byte [gdt64_tss + 5], 0x89

    ; Putting flag
    mov rax, 0x0
    or byte [gdt64_tss + 6], al

    ; Enabling TSS
    mov rax, kernel_stack_top
    mov [global_tss + 4], rax
    mov ax, 104
    mov word [global_tss + 102], ax
    mov ax, 0x0028
    ltr ax

    mov rax, cr4
    or  rax, (1 << 9) | (1 << 10)
    mov cr4, rax

    mov rax, cr0
    and rax, ~(1 << 2)
    or  rax, (1 << 1)
    mov cr0, rax

    push 0
    popfq

    ; RDI = magic, RSI = mbi_addr
    mov edi, [magic]
    mov esi, [mbi]

    call multiboot2_main

.hang:
    cli
    hlt
    jmp .hang
