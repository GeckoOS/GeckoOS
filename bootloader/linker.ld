/* 
 * bootloader will be at 0x7e00
 */
ENTRY(_entry)

SECTIONS
{
    . = 0x8000;

    .text   : { 
        *(.text.entry)
        *(.text)   
    }
    
    .rodata : { *(.rodata) }
    .data   : { *(.data)   }
    .bss    : { *(.bss) *(COMMON) }
}
