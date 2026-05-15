/*
 * kernel/boot/multiboot2_entry.c
 *
 * Called by multiboot2_entry (assembly) immediately after GRUB hands off
 * control.  Responsibilities:
 *
 *   1. Verify the Multiboot2 magic value so we panic early if launched by a
 *      non-compliant bootloader.
 *   2. Store the MBI pointer in a well-known global so the rest of the kernel
 *      can access it (especially the memory manager when it is wired up in
 *      Phase 1).
 *   3. Call the existing _entry() so the current kernel boots unchanged.
 *
 * Nothing here touches hardware directly; that is left to the subsystems
 * initialised by _entry().
 */

#include <boot/multiboot2.h>
#include <stdint.h>

/* Raw VGA color constants — used before the VGA driver is initialised. */
#define VGA_COLOR_BLACK     0
#define VGA_COLOR_LIGHT_RED 12

/* ── Exported globals ────────────────────────────────────────────────────── */

/**
 * Physical address of the Multiboot2 Boot Information structure.
 *
 * Set once by multiboot2_main() before _entry() is called.  Valid for the
 * lifetime of the kernel.  Phase 1 memory-management code should read this
 * to obtain the E820-equivalent memory map instead of the BIOS stub.
 */
uint32_t g_mbi_addr = 0;

/**
 * Copy of the bootloader magic written here so any subsystem can verify it
 * was a valid Multiboot2 boot without needing the original EAX value.
 */
uint32_t g_multiboot2_magic = 0;

/* ── Forward declaration ─────────────────────────────────────────────────── */

/* Existing kernel entry point — defined in kernel/kernel.c */
extern void _entry(void);

/* ── Entry ───────────────────────────────────────────────────────────────── */

/**
 * multiboot2_main - first C code to run after GRUB.
 *
 * @magic:    Value from EAX — should equal MULTIBOOT2_BOOTLOADER_MAGIC.
 * @mbi_addr: Value from EBX — physical address of the MBI structure.
 *
 * Called from multiboot2_entry.s with interrupts disabled and paging off.
 */
void multiboot2_main(uint32_t magic, uint32_t mbi_addr)
{
    g_multiboot2_magic = magic;
    g_mbi_addr         = mbi_addr;

    /*
     * Validate the magic value before touching anything else.
     *
     * VGA memory is always available at 0xB8000 in protected mode, so we can
     * write a raw error indicator even before the VGA driver is initialised.
     * We write 'MB' in red-on-black at the top-left corner.
     */
    if (magic != MULTIBOOT2_BOOTLOADER_MAGIC) {
        /* Write 'B''A''D'' ''M''A''G''I''C' directly to VGA text buffer. */
        volatile uint16_t *vga = (volatile uint16_t *)0xB8000;
        const char msg[] = "BAD MULTIBOOT2 MAGIC";
        uint8_t attr = (VGA_COLOR_BLACK << 4) | VGA_COLOR_LIGHT_RED;
        for (int i = 0; msg[i]; i++) {
            vga[i] = (uint16_t)((attr << 8) | (uint8_t)msg[i]);
        }
        /* Halt — we cannot continue safely. */
        for (;;) {
            __asm__ volatile ("cli; hlt");
        }
    }

    /*
     * Hand off to the existing kernel entry point.
     *
     * TODO(multiboot2): Before this call, Phase 1 should initialise the
     * memory manager using the MMAP tag from the MBI (g_mbi_addr) instead
     * of the BIOS E820 stub.  For now, _entry() calls kalloc_init() which
     * still uses the hardcoded 3 MB heap window — that is acceptable until
     * Phase 1 is complete.
     */
    _entry();

    /* Should never return. */
    for (;;) {
        __asm__ volatile ("cli; hlt");
    }
}