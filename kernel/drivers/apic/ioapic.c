#include "drivers/apic/ioapic.h"
#include "drivers/acpi.h"
#include "drivers/apic/lapic.h"
#include "drivers/ps2keyboard.h"
#include "drivers/vga.h"
#include "mem/paging.h"
#include "ports.h"
#include "terminal/printf.h"
#include <stdint.h>

//the modified virtual adress by virtual mapping
volatile uint32_t *ioapic;
//the original physical base given by the madt table
extern uintptr_t acpi_ioapic_base;

//reading a register form the ioapic
uint32_t ioapic_read(uint32_t reg)
{
    ioapic[IOAPIC_IOREGSEL / 4] = reg;
    return ioapic[IOAPIC_IOWIN / 4];
}
//writing a register to the ioapic
void ioapic_write(uint32_t reg, uint32_t value)
{
    ioapic[IOAPIC_IOREGSEL / 4] = reg;
    ioapic[IOAPIC_IOWIN / 4]    = value;
}

void ioapic_set_entry(volatile uint32_t *base, uint8_t index, uint64_t data)
{
    ioapic_write(IOAPIC_REDTBL_BASE + index * 2, (uint32_t)data);
    ioapic_write(IOAPIC_REDTBL_BASE + index * 2 + 1, (uint32_t)(data >> 32));
}
//Debug function for outputing the data of an entry
#ifdef DEBUG
    void ioapic_dump_entry(int gsi) {
        set_printf_color(VGA_COLOR_MAGENTA);
            uint64_t entry = ioapic_read(IOAPIC_REDTBL_BASE + gsi * 2) |
                            ((uint64_t)ioapic_read(IOAPIC_REDTBL_BASE + gsi * 2 + 1) << 32);
            printf("GSI %d: 0x%016lx | vector=%u | mask=%u | trigger=%u | polarity=%u | dest=%u\n",
                gsi, entry,
                (uint32_t)(entry & 0xFF),            // Vector
                (uint32_t)((entry >> 16) & 1),       // Mask (0 = unmasked)
                (uint32_t)((entry >> 15) & 1),       // Trigger (0 = edge)
                (uint32_t)((entry >> 13) & 1),       // Polarity (0 = high)
                (uint32_t)(entry >> 56)              // Destination LAPIC ID
            );
        set_printf_color(VGA_COLOR_WHITE);
    }
#endif

void ioapic_init()
{

    // Map IOAPIC physical address to virtual memory
    if (acpi_ioapic_base == 0) {
        set_printf_color(VGA_COLOR_RED);
            printf("No IOAPIC found in ACPI tables");
        set_printf_color(VGA_COLOR_WHITE);
    }

    uintptr_t ioapic_virt = acpi_ioapic_base;

    vmm_map(vmm_get_pml4(), acpi_ioapic_base, ioapic_virt,
            PTE_WRITABLE | PTE_CACHE_DISABLE);

    ioapic = (volatile uint32_t *)ioapic_virt;

    // Get IOAPIC version and maximum redirection entries
    uint32_t version = ioapic_read(IOAPIC_REG_VER);
    uint32_t max_redir =(version >> 16) & 0xFF; // Max redirection entry index(pins on card)
    #ifdef DEBUG
        set_printf_color(VGA_COLOR_DARK_GREY);
            printf("IOAPIC: version=%x, max_redir=%u\n", version & 0xFF, max_redir);
        set_printf_color(VGA_COLOR_WHITE);
    #endif

    // Mask all redirection entries
    for (int i = 0; i <= max_redir; ++i) {
        ioapic_set_entry(ioapic, i, 1 << 16);
    }

    // uint32_t current_id = (ioapic_read(IOAPIC_REG_ID) >> 24) & 0xF;
    //printf("IOAPIC: current ID=%u\n", current_id);
    //printf("IOapci and lapic base:%x",ioapic_virt);
}

void ioapic_redirect_irq(int gsi, int vector, uint16_t flags)
{

    // Need to find which IOAPIC handles this GSI
    // For single IOAPIC, using GSI as pin
    uint8_t pin = gsi;

    uint8_t polarity = (flags & 0x3) == 3 ? 1 : 0;        // Active low if 3
    uint8_t trigger  = ((flags >> 2) & 0x3) == 3 ? 1 : 0; // Level if 3

    uint64_t entry =
        vector | (0 << 8) |                      // Delivery mode: Fixed
        (0 << 11) |                              // Destination mode: Physical
        ((uint64_t)polarity << 13) | (0 << 14) | // Remote IRR (0 for now)
        ((uint64_t)trigger << 15) | (0 << 16) |  // Masked? 0 = unmasked
        ((uint64_t)lapic_get_id() << 56);                  
    
    ioapic_set_entry((uint32_t *)ioapic, pin, entry);
   
    // ioapic_dump_entry(gsi);
   
}
