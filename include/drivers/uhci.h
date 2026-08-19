#pragma once
//driver for usb devices

#include "drivers/pci.h"
#include "drivers/usb.h"
#include <stdint.h>

typedef uint32_t FrameEntry;

struct UHCIQueueHead {
    uint32_t horizontal_pointer;
    uint32_t vertical_pointer;
};
struct UHCITransferDescriptor {
    uint32_t next;
    uint32_t status;
    uint32_t packet_header;
    uint32_t buffer;
    uint32_t system_use;
};

struct UHCIDevice {
    struct BasicUSBHeader header;
    FrameEntry* framelist;
    struct UHCIQueueHead* qhpool;
    struct UHCITransferDescriptor* tdpool;
    int ports;
};

struct USBDevice uhci_init(struct PCIDevice device);