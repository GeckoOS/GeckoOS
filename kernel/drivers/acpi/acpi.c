#include "drivers/acpi.h"
#include "boot/multiboot2.h"
#include "drivers/vga.h"
#include "mem/paging.h"
#include "string.h"
#include "terminal/terminal.h"
#include <stddef.h>
#include <stdint.h>
#include <terminal/printf.h>
#include <drivers/tables/irq.h>
#include <drivers/pit.h>

struct madt_iso isos[32];
int isos_num = 0;

// Iterate trough multiboot info structure tags and search for rsdp v1 or v2
void *find_rsdp()
{

    /* uint8_t *mbi        = (uint8_t *)mbi_addr;
    uint32_t total_size = *(uint32_t *)mbi;

    multiboot2_tag_t *tag = (multiboot2_tag_t *)(mbi + 8);
    uintptr_t mbi_end     = (uintptr_t)mbi + total_size;

    while ((uintptr_t)tag < mbi_end && tag->type != MULTIBOOT2_TAG_TYPE_END) {
        // printf("tag at %p: type=%d size=%d\n", tag, tag->type, tag->size);

        // ONLY process ACPI tags - skip everything else
        if (tag->type == MULTIBOOT2_TAG_TYPE_ACPI_NEW) {
            multiboot2_tag_acpi_t *acpi = (multiboot2_tag_acpi_t *)tag;
            // printf("Found ACPI v2 RSDP at %p\n", acpi->rsdp);
            return acpi->rsdp;
        } else if (tag->type == MULTIBOOT2_TAG_TYPE_ACPI_OLD) {
            multiboot2_tag_acpi_t *acpi = (multiboot2_tag_acpi_t *)tag;
            // printf("Found ACPI v1 RSDP at %p\n", acpi->rsdp);
            return acpi->rsdp;
        } else {
            // Skip all other tag types

        }

        tag = (multiboot2_tag_t *)((uintptr_t)tag + ((tag->size + 7) & ~7));
    } */
    // The multiboot.c file can cover this
    extern struct multiboot2_tag_acpi* acpi_info_grub;

    // That tag from grub can be NULL
    // If that is the case, this will find the RSDP manually in memory

    if (!acpi_info_grub) goto manual;

    return acpi_info_grub->rsdp;
manual:
    for (uint64_t p = 0x000E0000; p < 0x000FFFFF; p += 16)
        if (*(uint64_t*)p == 0x5253442050545220) return (void*)p;
    // If nothing was found, keep searching from 0x40E to 0x40E + 1KIB (EBDA memory),
    for (uint64_t p = (0x40E * 0x10) & 0x000FFFFF; p < 0x40E + 1024; p += 0x10 / sizeof(p))
        if (*(uint64_t*)p == 0x5253442050545220) return (void*)p;
nothing:
    set_printf_color(VGA_COLOR_RED);
        printf("No ACPI tag found!\n");
    set_printf_color(VGA_COLOR_WHITE);
    return NULL;
}

uintptr_t acpi_lapic_base  = 0;
uintptr_t acpi_ioapic_base = 0;
// Find MADT via XSDT (ACPI 2.0+)
static struct acpi_madt *find_madt_via_xsdt(uint64_t xsdt_phys)
{
    // Map just the header first to get length
    struct acpi_header *header =
        (struct acpi_header *)mmio_map(xsdt_phys, sizeof(struct acpi_header));
    if (!header || *(uint32_t*)header->signature != 0x54445358) {
        return NULL;
    }

    uint32_t length = header->length;
    mmio_unmap((uintptr_t)header, sizeof(struct acpi_header));

    // Map the entire XSDT
    struct acpi_header *xsdt =
        (struct acpi_header *)mmio_map(xsdt_phys, length);
    if (!xsdt)
        return NULL;

    // Number of entries (each is 8 bytes for XSDT)
    int count = (xsdt->length - sizeof(struct acpi_header)) / 8;
    uint64_t *entries =
        (uint64_t *)((uint8_t *)xsdt + sizeof(struct acpi_header));

    struct acpi_madt *madt = NULL;

    for (int i = 0; i < count; i++) {
        // Map each table header to check signature
        struct acpi_header *hdr = (struct acpi_header *)mmio_map(
            entries[i], sizeof(struct acpi_header));
        if (!hdr)
            continue;

        if (*(uint32_t*)hdr->signature == 0x43495041) {
            // Found MADT - map the entire table
            uint32_t madt_len = hdr->length;
            mmio_unmap((uintptr_t)hdr, sizeof(struct acpi_header));

            madt = (struct acpi_madt *)mmio_map(entries[i], madt_len);
            break;
        }

        mmio_unmap((uintptr_t)hdr, sizeof(struct acpi_header));
    }

    mmio_unmap((uintptr_t)xsdt, length);
    return madt;
}

// Find MADT via RSDT (ACPI 1.0)
static struct acpi_madt *find_madt_via_rsdt(uint32_t rsdt_phys)
{
    // Map just the header first
    struct acpi_header *header =
        (struct acpi_header *)mmio_map(rsdt_phys, sizeof(struct acpi_header));
    if (!header || *(uint32_t*)header->signature != 0x54445352) {
        return NULL;
    }

    uint32_t length = header->length;
    mmio_unmap((uintptr_t)header, sizeof(struct acpi_header));

    // Map the entire RSDT
    struct acpi_header *rsdt =
        (struct acpi_header *)mmio_map(rsdt_phys, length);
    if (!rsdt)
        return NULL;

    // Number of entries (each is 4 bytes for RSDT)
    int count = (rsdt->length - sizeof(struct acpi_header)) / 4;
    uint32_t *entries =
        (uint32_t *)((uint8_t *)rsdt + sizeof(struct acpi_header));

    struct acpi_madt *madt = NULL;

    for (int i = 0; i < count; i++) {
        // Map each table header to check signature
        struct acpi_header *hdr = (struct acpi_header *)mmio_map(
            entries[i], sizeof(struct acpi_header));
        if (!hdr)
            continue;

        if (*(uint32_t*)hdr->signature == 0x43495041) {
            // Found MADT - map the entire table
            uint32_t madt_len = hdr->length;
            mmio_unmap((uintptr_t)hdr, sizeof(struct acpi_header));

            madt = (struct acpi_madt *)mmio_map(entries[i], madt_len);
            break;
        }

        mmio_unmap((uintptr_t)hdr, sizeof(struct acpi_header));
    }

    mmio_unmap((uintptr_t)rsdt, length);
    return madt;
}

// Parse MADT entries
static int parse_madt_entries(struct acpi_madt *madt)
{
    if (!madt)
        return 0;

    // Map LAPIC base
    acpi_lapic_base =madt->lapic_addr;
    #ifdef DEBUG
        set_printf_color(VGA_COLOR_DARK_GREY);
            printf("LAPIC is at %p\n", acpi_lapic_base);
        set_printf_color(VGA_COLOR_WHITE);
    #endif

    // Parse entries after the MADT header
    uint8_t *ptr = (uint8_t *)madt + sizeof(struct acpi_madt);
    uint8_t *end = (uint8_t *)madt + madt->header.length;

    int found_ioapic = 0;

    while (ptr < end) {
        struct madt_entry_header *h = (struct madt_entry_header *)ptr;

        // Sanity check
        if (h->length == 0 || ptr + h->length > end) {
            // printf("Invalid MADT entry at %p (length %d)\n", ptr, h->length);
            break;
        }

        switch (h->type) {
/*         case 0: // LAPIC
                //   printf("Found LAPIC at entry %d\n", found_ioapic);
            break; */

        case 1: { // IOAPIC
            struct madt_ioapic *io = (struct madt_ioapic *)ptr;
            acpi_ioapic_base       = io->addr;//mmio_map(io->addr, PAGE_SIZE);
            #ifdef DEBUG
                set_printf_color(VGA_COLOR_DARK_GREY);
                    printf("IOAPIC found: addr=%p, gsi_base=%u\n", acpi_ioapic_base,
                        io->gsi_base);
                set_printf_color(VGA_COLOR_WHITE);
            #endif
            found_ioapic = 1;
            break;
        }

         case 2: { // ISO (Interrupt Source Override)
            struct madt_iso *iso = (struct madt_iso *)ptr;
            // printf("ISO: bus=%u, irq=%u, gsi=%u, flags=0x%x\n",
            //       iso->bus,iso->irq, iso->gsi, iso->flags);
            isos[isos_num++] = *(struct madt_iso *)ptr;
            break;
        }
/*         case 3: {
            break;
        }
        case 4: {
            break;
        }
        case 5: {
            break;
        } */
        default:
            // printf("Unknown MADT entry type %d\n", h->type);
            break;
        }

        ptr += h->length;
    }

    return found_ioapic;
}

acpi_fadt_t *fadt = NULL;

// Function that does the init for acpi and other stuff that needs parsing from
// mbi_addr including lapic and ioapic
int acpi_init()
{
    // printf("=== ACPI INIT START ===\n");
    // printf("magic=0x%lx, mbi_addr=0x%lx\n", magic, mbi_addr);

    struct acpi_rsdp_v1 *rsdp =
        (struct acpi_rsdp_v1 *)find_rsdp();

    if (!rsdp) {
        set_printf_color(VGA_COLOR_RED);
            printf("FAIL: No RSDP found\n");
        set_printf_color(VGA_COLOR_WHITE);
        return -1;
    }
    // debuging forgot to enable maping hate this shit
    //  printf("RSDP found at %p\n", rsdp);
    //  printf("RSDP revision: %d\n", rsdp->revision);
    //  printf("RSDP OEM ID: %.6s\n", rsdp->oem_id);
    //  printf("RSDT addr: 0x%x\n", rsdp->rsdt_addr);
    // check if revision exists we move to v2
/*     if (rsdp->revision >= 2) {
        struct acpi_rsdp_v2 *rsdp_v2 = (struct acpi_rsdp_v2 *)rsdp;
        // printf("XSDT addr: 0x%lx\n", rsdp_v2->xsdt_addr);
        // printf("RSDP length: %d\n", rsdp_v2->length);
    } */

    struct acpi_madt *madt = NULL;

    // Try XSDT first v2 if revision exists
    if (rsdp->revision >= 2) {
        struct acpi_rsdp_v2 *rsdp_v2 = (struct acpi_rsdp_v2 *)rsdp;
        if (rsdp_v2->xsdt_addr != 0) {
            //   printf("\nTrying XSDT at 0x%lx\n", rsdp_v2->xsdt_addr);

            // Map XSDT header
            struct acpi_header *xsdt_header = (struct acpi_header *)mmio_map(
                rsdp_v2->xsdt_addr, sizeof(struct acpi_header));
            if (!xsdt_header) {
                set_printf_color(VGA_COLOR_RED);
                    printf("FAIL: Could not map XSDT header\n");
                set_printf_color(VGA_COLOR_WHITE);
            } else {
                if (*(uint32_t*)xsdt_header->signature == 0x54445358) {
                    // Map full XSDT
                    struct acpi_header *xsdt = (struct acpi_header *)mmio_map(
                        rsdp_v2->xsdt_addr, xsdt_header->length);
                    if (xsdt) {
                        int count =
                            (xsdt->length - sizeof(struct acpi_header)) / 8;
                        // printf("XSDT has %d entries\n", count);

                        uint64_t *entries =
                            (uint64_t *)((uint8_t *)xsdt +
                                         sizeof(struct acpi_header));
                        for (int i = 0; i < count; i++) {
                            // printf("  Entry %d: 0x%lx\n", i,
                            // entries[i]);

                            // Map just the header of each table
                            struct acpi_header *hdr =
                                (struct acpi_header *)mmio_map(
                                    entries[i], sizeof(struct acpi_header));
                            if (!hdr) {
                                set_printf_color(VGA_COLOR_RED);
                                    printf("FAIL: Could notmap header\n");
                                set_printf_color(VGA_COLOR_WHITE);
                                continue;
                            }
                            uint32_t length = hdr->length;
                            switch (*(uint32_t*)hdr->signature) {
                                case 0x43495041: // MADT
                                    mmio_unmap((uintptr_t)hdr,
                                           sizeof(struct acpi_header));
                                    // Map full MADT
                                    madt = (struct acpi_madt *)mmio_map(
                                        entries[i], length);
                                    break;
                                case 0x50434141: // FADT
                                    mmio_unmap((uintptr_t)hdr,
                                           sizeof(struct acpi_header));
                                    // Map full FADT
                                    fadt = (acpi_fadt_t *)mmio_map(
                                        entries[i], length);
                                    break;
                                default:
                                    break;
                            }

                            // Headers are only mapped not unmapped memory
                            // deallocation is for losers
                        }
                    }
                } else {
                    set_printf_color(VGA_COLOR_RED);
                        printf("Invalid XSDT signature\n");
                    set_printf_color(VGA_COLOR_WHITE);
                }
            }
        }
    }

    // Try RSDT fallback
    if (!madt && rsdp->rsdt_addr != 0) {
        struct acpi_header *rsdt_header = (struct acpi_header *)mmio_map(
            rsdp->rsdt_addr, sizeof(struct acpi_header));
        // printf("\n %p \n", rsdt_header);
        if (rsdt_header) {
            //   printf("RSDT signature: %.4s\n", rsdt_header->signature);
            // printf("RSDT length: %u\n", rsdt_header->length);

            if (*(uint32_t*)rsdt_header->signature == 0x54445352) {
                struct acpi_header *rsdt = (struct acpi_header *)mmio_map(
                    rsdp->rsdt_addr, rsdt_header->length);
                if (rsdt) {
                    int count = (rsdt->length - sizeof(struct acpi_header)) / 4;
                    // printf("RSDT has %d entries\n", count);
                    uint32_t *entries =
                        (uint32_t *)((uint8_t *)rsdt +
                                     sizeof(struct acpi_header));

                    for (int i = 0; i < count; i++) {
                        // printf("  %d: 0x%x\n", i, entries[i]);

                        struct acpi_header *hdr =
                            (struct acpi_header *)mmio_map(
                                entries[i], sizeof(struct acpi_header));
                        if (!hdr)
                            continue;
                        switch (*(uint32_t*)hdr->signature) {
                            case 0x43495041: // MADT
                                madt = (struct acpi_madt *)mmio_map(
                                    entries[i], hdr->length);
                                break;
                            case 0x50434146: // FADT
                                fadt = (acpi_fadt_t *)mmio_map(
                                    entries[i], hdr->length);
                                
/*                                 struct acpi_header* dsdt = (struct acpi_header*)mmio_map((uint64_t)fadt->dsdt, sizeof(struct acpi_header));
                                uint32_t amllenght = dsdt->length - sizeof(struct acpi_header);
                                uint8_t* amlbytes = (uint8_t*)mmio_map((uint64_t)(dsdt + sizeof(struct acpi_header)), amllenght);

                                uint32_t* S5Addr;
                                for (uint32_t i = 0; i < amllenght; i++) {
                                    printf("%p\n", &amlbytes[i]);
                                    if (*((uint32_t*)&amlbytes[i]) == 0x5F35535F) break;
                                } */
                                break;
                            case 0x54445344:
                                    printf("HEREEE\n"); // there is no dsdt in qemu
                                break;
                            default:
                                break;
                        }
                    }
                }
            }
        }
    }

    if (!madt) {
        set_printf_color(VGA_COLOR_LIGHT_RED);
            printf("\nFAIL: MADT not found in any table\n");
            printf("This means either:\n");
            printf("  1. ACPI tables are corrupted\n");
            printf("  2. Memory mapping is failing\n");
            printf("  3. Your MMIO mapping doesn't work for physical addresses > "
                "4GB\n");
            printf("  4. find_rsdp() returned a pointer to unmapped memory\n");
        set_printf_color(VGA_COLOR_WHITE);
        return -2;
    }

    printc("MADT found successfully!\n", VGA_COLOR_LIGHT_GREY);
    // printf("LAPIC address: 0x%x\n", madt->lapic_addr);

    parse_madt_entries(madt);
    // printf("LAPIC mapped at %p\n", acpi_lapic_base);
    
    // activate the acpi
/*     if ((inw(fadt->pm1aControlBlk) & 1) == 0) {
        if (fadt->smiCommandPort != 0 && fadt->acpiEnable != 0) {
            outb(fadt->smiCommandPort, fadt->acpiEnable);

            uint16_t i;
            for (i = 0; i < 300; i++) {
                if ( (inw((unsigned int) fadt->pm1aControlBlk) & 1) == 1 )
                    break;
                sleep(1);
            }
            if (fadt->pm1bControlBlk != 0)
                for (int i = 0 ; i < 300; i++) {
                    if ( (inw((unsigned int) fadt->pm1bControlBlk) & 1) == 1 )
                        break;
                    sleep(1);
                }
        } else {
            return -1;
        }
    } */

    return 0;
}
int shutdown() {
    if (!fadt) return -1;

    // Someday...

    return 0;
}