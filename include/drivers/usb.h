#pragma once

#include "drivers/pci.h"
#include <stdint.h>

#define UHCICONTROLLER 1
#define OHCICONTROLLER 2
#define EHCICONTROLLER 3
#define xHCICONTROLLER 4

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

struct usb_setup_packet {
    uint8_t requesttype;
    uint8_t request;
    uint16_t value;
    uint16_t index;
    uint16_t lenght;
};

extern struct USBDevice USBDevices[16];
extern uint8_t USBDevices_Count;

static const char* USBTypesTable[] = {
    "No type",
    "UHCI",
    "OHCI",
    "EHCI",
    "xHCI"
};