#include "mem.h"
#include <drivers/usb.h>
#include <terminal/printf.h>
#include <stdint.h>

struct USBDevice USBDevices[16];
uint8_t USBDevices_Count = 0;

bool IsHID(struct USBDevice* device) {
    printf(" P%d\n", device->data.device_address);
    device->SendPacket(&device->data, (struct usb_setup_packet){
        .requesttype = 0x80,
        .request = REQUEST_GET_DESCRIPTOR,
        .value = DESCRIPTOR_TYPE_CONFIGURATION << 8,
        .index = 0,
        .lenght = (sizeof(device->data.config_descriptor.header) + sizeof(device->data.config_descriptor.wTotalLength))
    }, &device->data.config_descriptor.header, false);
    printf(" P%d\n", device->data.device_address);

    char* buffer = kmalloc(device->data.config_descriptor.wTotalLength);
    printf(" P%d\n", device->data.config_descriptor.wTotalLength);
    device->SendPacket(&device->data, (struct usb_setup_packet){
        .requesttype = 0x80,
        .request = REQUEST_GET_DESCRIPTOR,
        .value = DESCRIPTOR_TYPE_CONFIGURATION << 8,
        .index = 0,
        .lenght = device->data.config_descriptor.wTotalLength
    }, buffer, false);
    for(;;);

    int i = sizeof(device->data.config_descriptor);
    while (buffer[i]) {
        const struct usb_descriptor_head* header = (struct usb_descriptor_head*)&buffer[i];

        printf("type: %d\n", header->bDescriptortype);

        break;
    }

    if ((device->data.device_descriptor.bDeviceclass == 0) && (device->data.device_descriptor.bSubdeviceclass == 0)) {
        return true;
    } return false;
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