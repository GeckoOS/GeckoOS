ENTRY(_entry)
SECTIONS
{
    /* position the linker at 1M */
    . = 1M;

    /* executable data */
    .text : ALIGN(4K)
    {
        *(.multiboot)
        *(.text)
    }

    /* Read-only data */
    .rodata : ALIGN(4K)
    {
        *(.rodata)
    }

    /* Read-write data (initialized) */
    .data : ALIGN(4K)
    {
        *(.data)
    }

    /* Read-write data (uninitialized) */
    .bss : ALIGN(4K)
    {
        __bss_end__ = .;
        *(COMMON)
        *(.bss)
        __bss_start__ = .;
    }
    end = .;
}