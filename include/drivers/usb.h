#pragma once
//driver for usb devices

#include "drivers/pci.h"
#include <stdint.h>




struct usb_controller {
    struct pci_common_hdr pci;

    volatile uint8_t* mmio_base;

    uint8_t type; // UHCI/OHCI/EHCI/xHCI

    void* operational_regs;
    void* runtime_regs;

    uint8_t irq;
};
void usb_init();