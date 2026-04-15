/* By Ember2819, your welcome dorito girl. Took me way too long to figure out how to make this... */

ENTRY(_entry)

SECTIONS
{
    . = 0x100000;

    .text   : { 
        *(.text.entry)
        *(.text)
        . = ALIGN(4096);
    }
    
    .rodata : { *(.rodata) . = ALIGN(4096); }
    .data   : { *(.data) . = ALIGN(4096);   }
    .bss    : {
        *(.bss)
        *(COMMON)
        . = ALIGN(4096);
    }
    end = .;
}
