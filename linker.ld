OUTPUT_FORMAT(elf64-x86-64)
ENTRY(multiboot2_entry)

PHYS_BASE = 0x100000;
VIRT_BASE = 0xFFFFFFFF80000000;

PHDRS
{
    text PT_LOAD FLAGS(5);  /* R-X */
    data PT_LOAD FLAGS(6);  /* RW- */
}

SECTIONS
{
    . = PHYS_BASE;

    .multiboot2_header : {
        KEEP(*(.multiboot2_header))
    } :text

    .multiboot1_header : {
        KEEP(*(.multiboot1_header))
    } :text

    .text : {
        *(.text.multiboot2_entry)
        *(.text.entry)
        *(.text*)
    } :text

    .rodata : {
        *(.rodata*)
    } :text

    .data : {
        *(.data*)
    } :data

    .bootstrap_data (NOLOAD) : {
        *(.bootstrap_data)
    } :data

    .bss : {
        _bss_start = .;
        *(.bss*)
        *(COMMON)
        _bss_end = .;
    } :data
}