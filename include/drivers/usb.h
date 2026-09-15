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

// HID requests
#define REQUEST_SET_PROTOCOL 0x0B
#define REQUEST_GET_REPORT 1
// HID request types
#define REQUEST_TYPE_SET_PROTOCOL 0x21
#define REQUEST_TYPE_GET_REPORT 0xA1

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

// Endpoint descriptor struct
struct usb_endpoint_descriptor {
    struct usb_descriptor_head header;
    uint8_t bEndpointAddress;
    uint8_t bmAttributes;
    uint16_t wMaxPacketSize;
    uint8_t bInterval;
} __attribute__((packed));

struct BasicUSBHeader {
    struct usb_device_descriptor device_descriptor;

    // Arrays
    struct usb_configuration_descriptor config_descriptor[8];
    uint8_t config_count;
    struct usb_interface_descriptor interface[16];
    uint8_t interfaces_count;
    struct usb_endpoint_descriptor endpoint[20];
    uint8_t endpoints_count;

    struct PCIDevice* controller;
    void* controller_reserved; // Used by the controller

    void* user_data; // Used by HID Devices and its data
};

#define INTERRUPT_TRANSFER_INTERVAL_1MS 0
#define INTERRUPT_TRANSFER_INTERVAL_2MS 2
#define INTERRUPT_TRANSFER_INTERVAL_4MS 4
#define INTERRUPT_TRANSFER_INTERVAL_8MS 6
#define INTERRUPT_TRANSFER_INTERVAL_16MS 8
#define INTERRUPT_TRANSFER_INTERVAL_32MS 10

struct USBDevice {
    uint8_t type;
    struct BasicUSBHeader data;
    bool is_hid;
    uint8_t index;

    bool (*SendPacket)(struct BasicUSBHeader* data, struct usb_setup_packet setup_packet, void* buffer, bool no_response /* If the packet has no response */);
    void (*InitInterruptTranfers)(struct BasicUSBHeader* data);
    void (*SetInterruptTransfer)(struct BasicUSBHeader* data, uint8_t interval, void* buffer, uint16_t size);
};

/*
A UHCI Controller can have 2 usbs max, so
*/

extern struct USBDevice USBDevices[16];
extern uint8_t USBDevices_Count;

static const char* USBTypesTable[] = {
    "No type",
    "UHCI",
    "OHCI",
    "EHCI",
    "xHCI",
    "PS2" // hufdehiugdfiuh
};

bool IsHID(struct USBDevice* device);
bool SendUSBPacket(struct USBDevice* device, struct usb_setup_packet packet, void* buffer);
void GetUSBStringIndex(struct USBDevice device, struct usb_string_descriptor* string, uint8_t index, uint16_t langid);
void GetUSBConfiguration(struct USBDevice* device, uint8_t configuration_index);
