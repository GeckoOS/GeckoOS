#include "drivers/usb.h"
#include <drivers/uhci.h>
#include <stdint.h>
#include <terminal/printf.h>

struct KeyboardReport {
    uint8_t modifier_key_status;
    uint8_t reserved;
    uint8_t keypresses[6];
} __attribute__((packed));

void SetProtocol(struct USBDevice usb, uint16_t protocol) {
    usb.SendPacket(&usb.data, (struct usb_setup_packet){
        .requesttype = REQUEST_TYPE_SET_PROTOCOL,
        .request = REQUEST_SET_PROTOCOL,
        .value = protocol,
        .index = 0,
        .lenght = 0
    }, NULL, true);
}
void GetReport(struct USBDevice usb) {
    struct KeyboardReport buffer;

    usb.SendPacket(&usb.data, (struct usb_setup_packet){
        .requesttype = REQUEST_TYPE_GET_REPORT,
        .request = REQUEST_GET_REPORT,
        .value = 0x0100,
        .index = 0,
        .lenght = sizeof(struct KeyboardReport)
    }, &buffer, false);

    printf("%x\n", buffer.keypresses[0]);
}