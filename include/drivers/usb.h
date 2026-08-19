#pragma once

#include "drivers/pci.h"
#include <stdint.h>

#define UHCICONTROLLER 1

struct BasicUSBHeader {
    struct PCIDevice device;
    uint16_t ioport;
};

struct USBDevice {
    uint8_t type;
    struct BasicUSBHeader* data;
    void (*handler)(struct BasicUSBHeader* data);
    int irq;
};

extern struct USBDevice USBDevices[16];
extern uint8_t USBDevices_Count;