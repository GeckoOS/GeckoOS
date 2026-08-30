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

// Descriptors
struct usb_setup_packet {
    uint8_t requesttype;
    uint8_t request;
    uint16_t value;
    uint16_t index;
    uint16_t lenght;
} __attribute__((packed));

struct usb_descriptor_head {
    uint8_t bLength; // Size of this descriptor in bytes
    uint8_t bDescriptortype; // Type of this descriptor
} __attribute__((packed));

// Device descriptor
struct usb_device_descriptor {
    struct usb_descriptor_head header;
    uint16_t bcdUSB;
    uint8_t bDeviceclass;
    uint8_t bSubdeviceclass;
    uint8_t bDeviceprotocol;
    uint8_t bMaxpacketsize;
    uint16_t idVendor;
    uint16_t idProduct;
    uint16_t bcdDevice;
    uint8_t iManufacter;
    uint8_t iProduct;
    uint8_t iSerialnumber;
    uint8_t bNumConfigurations;
} __attribute__((packed));

// String descriptor structs
struct usb_string_descriptor {
    uint8_t blength;
    uint8_t btype;
    uint8_t string[0xFF];
} __attribute__((packed));

extern struct USBDevice USBDevices[16];
extern uint8_t USBDevices_Count;

static const char* USBTypesTable[] = {
    "No type",
    "UHCI",
    "OHCI",
    "EHCI",
    "xHCI"
};