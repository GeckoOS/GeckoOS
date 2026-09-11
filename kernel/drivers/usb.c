#include "mem.h"
#include <drivers/usb.h>
#include <stdbool.h>
#include <terminal/printf.h>
#include <stdint.h>

struct USBDevice USBDevices[16];
uint8_t USBDevices_Count = 0;

void GetUSBConfiguration(struct USBDevice* device, uint8_t configuration_index) { // Put the configuration descriptor and all of its interfaces and endpoints inside the usb device struct
    device->SendPacket(&device->data, (struct usb_setup_packet){
        .requesttype = 0x80,
        .request = REQUEST_GET_DESCRIPTOR,
        .value = DESCRIPTOR_TYPE_CONFIGURATION << 8,
        .index = 0,
        .lenght = (sizeof(device->data.config_descriptor[0].header) + sizeof(device->data.config_descriptor[0].wTotalLength))
    }, &device->data.config_descriptor[0].header, false);

    char* buffer = kmalloc(device->data.config_descriptor[0].wTotalLength);
    device->SendPacket(&device->data, (struct usb_setup_packet){
        .requesttype = 0x80,
        .request = REQUEST_GET_DESCRIPTOR,
        .value = (DESCRIPTOR_TYPE_CONFIGURATION << 8) | configuration_index,
        .index = 0,
        .lenght = device->data.config_descriptor[0].wTotalLength
    }, buffer, false);
    
    for (int i = sizeof(struct usb_configuration_descriptor); i < device->data.config_descriptor[0].wTotalLength;) {
        const struct usb_descriptor_head* header = (struct usb_descriptor_head*)&buffer[i];

        if (header->bDescriptortype == DESCRIPTOR_TYPE_INTERFACE) {
            const struct usb_interface_descriptor* interface = (struct usb_interface_descriptor*)header;
            device->data.interface[device->data.interfaces_count++] = *interface;
        } else if (header->bDescriptortype == DESCRIPTOR_TYPE_ENDPOINT) {
            const struct usb_endpoint_descriptor* endpoint = (struct usb_endpoint_descriptor*)header;
            device->data.endpoint[device->data.endpoints_count++] = *endpoint;
        }

        i += header->bLength;
    }
    kfree(buffer);
}

bool IsHID(struct USBDevice* device) {
    if ((device->data.device_descriptor.bDeviceclass != 0) || (device->data.device_descriptor.bSubdeviceclass != 0)) return false;
    if (device->data.interface[0].bInterfaceClass && device->data.interface[0].bInterfaceSubClass) return true;
    return false;
}

bool GetUSBDescriptor(struct USBDevice* device, struct usb_setup_packet packet, void* buffer) { return device->SendPacket(&device->data, packet, buffer, false); }

void GetUSBStringIndex(struct USBDevice device, struct usb_string_descriptor* string, uint8_t index, uint16_t langid) {
    GetUSBDescriptor(&device, (struct usb_setup_packet){
        .requesttype = 0x80,
        .request = REQUEST_GET_DESCRIPTOR,
        .value = (DESCRIPTOR_TYPE_STRING << 8) | index,
        .index = langid,
        .lenght = sizeof(string->header.bLength)
    }, string);
    GetUSBDescriptor(&device, (struct usb_setup_packet){
        .requesttype = 0x80,
        .request = REQUEST_GET_DESCRIPTOR,
        .value = (DESCRIPTOR_TYPE_STRING << 8) | index,
        .index = langid,
        .lenght = string->header.bLength
    }, string);
}