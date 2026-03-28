;
; bonk enjoyer (dorito girl)
;
; Organize the kernel at 0x10000 for this to work properly
;
; Created, because grub will probably be too complex for the "owner" of this repo
;
;

org 0x7c00
bits 16

_start:
; load few more sectors of bootloader, we need them
	mov ah, 0x02
	mov al, 3
	mov ch, 0x0
	mov cl, 0x2
	mov dh, 0x0
	mov dl, 0x80
	mov bx, 0x7e00
	mov si, 0
	mov es, si
	int 0x13

; load the c part
mov ah, 0x02
	mov al, 3
	mov ch, 0x0
	mov cl, 0x5
	mov dh, 0x0
	mov dl, 0x80
	mov bx, 0x0
	mov si, 0x1000
	mov es, si
	int 0x13

; get memory info from bios

boot_2:
	cli
	lgdt	[gdt_descriptor]

	mov	eax, cr0
	or	eax, 1
	mov	cr0, eax

	;; Stack

	mov	esp, 0x900000

	jmp	0x8:protected_mode_exec	

BOOT_DRIVE	db 0x0

;; GDT
bits 32

protected_mode_exec:
	mov	ax, 0x10
	mov	ds, ax
	mov	es, ax
	mov	es, ax
	mov	ss, ax
	mov	fs, ax
	mov	gs, ax

	jmp	0x10000

times 446 - ($ - $$) db 0
	; MBR
	db 0x80
	db 0x0
	db 0x0
	db 0x0
	db 0xC
	db 0x0
	db 0x0
	db 0x0
	dd 0x0
	dd 0x800

	db 0x80
	db 0x0
	db 0x0
	db 0x0
	db 0xC
	db 0x0
	db 0x0
	db 0x0
	dd 0x800
	dd 0x1

times 510 - ($ - $$) db 0

;; Boot signature

dw 0xAA55

gdt_start:
gdt_null:
	dq	0x0
gdt_code:
	dw	0xFFFF
	dw	0x0
	db	0x0
	db	10011010b
	db	11001111b
	db	0x0
gdt_data:
	dw	0xFFFF
	dw	0x0
	db	0x0
	db	10010010b
	db	11001111b
	db	0x0
gdt_end:
gdt_descriptor:
	dw	gdt_end - gdt_start - 1
	dd	gdt_start

times 1024 - ($ - $$) db 0
; 0x8000
; our mappings are here

times 1800 - ($ - $$) db 0

bits 16
get_memory_map_info:

	mov si, 0x0
	mov es, si
	mov di, 0x8000 ; 0x0:0x8000
	xor ebx, ebx
	mov edx, 0x534D4150
	mov eax, 0xE820
	mov ecx, 24
	int 0x15
	ret


times 2048 - ($ - $$) db 0
