#include "drivers/pci.h"
#include "mem/paging.h"
#include <drivers/mass_storage_controller/ahci.h>
#include <stdint.h>
#include <terminal/printf.h>

#define ABAR_OFF 0x24

void ahci_init(struct PCIDevice device) {
    MemorySpaceBar Abar = PCIGetMemorySpaceBar(device, 5);
    if (!Abar) goto err;

    uint8_t* registers_address = (uint8_t*)((uint64_t)Abar & ~0xF); // 16-byte aligned address
    vmm_map(vmm_get_pml4(), (uint64_t)registers_address, (uint64_t)registers_address, PTE_PRESENT | PTE_WRITABLE | PTE_CACHE_DISABLE);

    printf("%x AHCI Address\n", *registers_address);

err:
    return;
}