/* By Ember2819, your welcome dorito girl. Took me way too long to figure out how to make this... */

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
    .bss    : {
        __bss_start = .;
        *(.bss)
        *(COMMON)
        __bss_end = .;
    }
}
