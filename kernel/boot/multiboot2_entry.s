bits 32

MULTIBOOT2_MAGIC    equ 0xE85250D6
MULTIBOOT2_ARCH     equ 0
HEADER_LENGTH       equ (mb2_header_end - mb2_header_start)
HEADER_CHECKSUM     equ -(MULTIBOOT2_MAGIC + MULTIBOOT2_ARCH + HEADER_LENGTH)

MULTIBOOT1_MAGIC    equ 0x1BADB002
MULTIBOOT1_FLAGS    equ 0x3
MULTIBOOT1_CHECKSUM equ -(MULTIBOOT1_MAGIC + MULTIBOOT1_FLAGS)

section .multiboot1_header
align 4
    dd MULTIBOOT1_MAGIC
    dd MULTIBOOT1_FLAGS
    dd MULTIBOOT1_CHECKSUM

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
gdt64_end:

gdt64_ptr:
    dw gdt64_end - gdt64_start - 1
    dq gdt64_start          ; 64-bit base

section .text.multiboot2_entry
global multiboot2_entry
extern multiboot2_main

multiboot2_entry:
    mov edi, eax
    mov esi, ebx

    mov ebp, eax

    mov esp, early_stack_top

    mov edi, pml4_table
    mov ecx, 4096
    xor eax, eax
    rep stosd
    mov edi, ebp            ; restore edi = magic

    mov ebx, pml4_table
    mov ecx, (4 * 4096) / 4
    xor eax, eax
.zero_tables:
    mov [ebx], eax
    add ebx, 4
    loop .zero_tables

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

    mov rax, cr4
    or  rax, (1 << 9) | (1 << 10)
    mov cr4, rax

    mov rax, cr0
    and rax, ~(1 << 2)
    or  rax, (1 << 1)
    mov cr0, rax

    mov rsp, kernel_stack_top

    push 0
    popfq

    ; RDI = magic, RSI = mbi_addr
    mov edi, edi      ; zero‑extends EDI into RDI
    mov esi, esi  

    call multiboot2_main

.hang:
    cli
    hlt
    jmp .hang
