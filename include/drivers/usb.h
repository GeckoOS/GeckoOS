#pragma once

#include "drivers/pci.h"
#include <stdbool.h>
#include <stdint.h>

#define UHCICONTROLLER 1
#define OHCICONTROLLER 2
#define EHCICONTROLLER 3
#define xHCICONTROLLER 4

// Standards requests
#define REQUEST_GET_STATUS 0
#define REQUEST_CLEAR_FEATURE 1
#define REQUEST_SET_FEATURE 3
#define REQUEST_SET_ADDRESS 5
#define REQUEST_GET_DESCRIPTOR 6
#define REQUEST_SET_DESCRIPTOR 7
#define REQUEST_GET_CONFIGURATION 8
#define REQUEST_SET_CONFIGURATION 9
#define REQUEST_GET_INTERFACE 10
#define REQUEST_SET_INTERFACE 11
#define REQUEST_SYNC_FRAME 12

// Standards descriptors
#define DESCRIPTOR_TYPE_DEVICE 1
#define DESCRIPTOR_TYPE_CONFIGURATION 2
#define DESCRIPTOR_TYPE_STRING 3
#define DESCRIPTOR_TYPE_INTERFACE 4
#define DESCRIPTOR_TYPE_ENDPOINT 5
#define DESCRIPTOR_TYPE_DEVICE_QUALIFIER 6
#define DESCRIPTOR_TYPE_OTHER_SPEED_CONFIGURATION 7
#define DESCRIPTOR_TYPE_INTERFACE_POWER 8

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

// String descriptor struct
struct usb_string_descriptor {
    struct usb_descriptor_head header;
    uint16_t string[0xFF];
} __attribute__((packed));

// Interface descriptor struct
struct usb_interface_descriptor {
    struct usb_descriptor_head header;
    uint8_t bInterfaceNumber;
    uint8_t bAlternateSetting;
    uint8_t bNumEndpoints;
    uint8_t bInterfaceClass;
    uint8_t bInterfaceSubClass;
    uint8_t bInterfaceProtocol;
    uint8_t iInterface;
} __attribute__((packed));

// Configuration descriptor struct
struct usb_configuration_descriptor {
    struct usb_descriptor_head header;
    uint16_t wTotalLength;
    uint8_t bNumInterfaces;
    uint8_t bConfigurationValue;
    uint8_t iConfiguration;
    uint8_t bmAttributes;
    uint8_t bMaxPower;
} __attribute__((packed));

struct BasicUSBHeader {
    struct PCIDevice device;
    struct usb_device_descriptor device_descriptor;
    bool is_hid;
    uint8_t device_address;
    uint16_t ioport;
};

struct USBDevice {
    uint8_t type;
    struct BasicUSBHeader* data;
    bool (*SendPacket)(struct BasicUSBHeader* data, struct usb_setup_packet setup_packet, void* buffer, bool two /* Two or three packets */);
    int irq;
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

bool IsHID(struct USBDevice device);
bool GetUSBDescriptor(struct USBDevice* device, struct usb_setup_packet packet, void* buffer);
void SetUSBAddress(struct USBDevice* device, uint8_t to);
void GetUSBStringIndex(struct USBDevice device, struct usb_string_descriptor* string, uint8_t index, uint16_t langid);