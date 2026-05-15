/*
 * linker.ld — GeckoOS kernel linker script
 *
 * Kernel is loaded at physical address 1 MB (0x100000), matching the
 * original layout.  The Multiboot2 header section is forced to the very
 * beginning of the image so GRUB can locate it within the first 32 KB.
 *
 * Original author: Ember2819
 * Multiboot2 section added as part of the GRUB migration (Phase 2 roadmap).
 */

ENTRY(multiboot2_entry)

SECTIONS
{
    . = 0x100000;

    /*
     * Multiboot2 header MUST appear within the first 32 KB of the image.
     * Placing it here guarantees it is the very first thing in the binary.
     */
    .multiboot2_header : {
        KEEP(*(.multiboot2_header))
    }

    /* Multiboot2 entry stub and existing kernel entry come next. */
    .text : {
        *(.text.multiboot2_entry)
        *(.text.entry)
        *(.text)
    }

    .rodata : { *(.rodata) }
    .data   : { *(.data)   }

    .bss : {
        *(.bss)
        *(COMMON)
    }
}
