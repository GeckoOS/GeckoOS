#include <drivers/usb.h>
#include <terminal/printf.h>
#include <stdint.h>

struct USBDevice USBDevices[16];
uint8_t USBDevices_Count = 0;

bool IsHID(struct USBDevice device) {
    for (int i = 0; i < device.data.device_descriptor.bNumConfigurations; i++) {
        struct usb_configuration_descriptor config;
        GetUSBDescriptor(&device, (struct usb_setup_packet){
            .requesttype = 0x80,
            .request = REQUEST_GET_INTERFACE,
            .value = DESCRIPTOR_TYPE_STRING << 8,
            .index = 0x00,
            .lenght = sizeof(config)
        }, &config);

        printf("Configuration: %x\n", config.bMaxPower);
    }

    if ((device.data.device_descriptor.bDeviceclass == 0) && (device.data.device_descriptor.bSubdeviceclass == 0)) {
        return false;
        GetUSBDescriptor(&device, (struct usb_setup_packet){
            .requesttype = 0x00,
            .request = REQUEST_GET_INTERFACE,
            .value = 0,
            .index = 0x00,
            .lenght = 0x00
        }, NULL);
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