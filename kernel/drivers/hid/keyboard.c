#include "drivers/hid/keyboard.h"
#include "drivers/input.h"
#include "drivers/pit.h"
#include "drivers/ps2keyboard.h"
#include "drivers/usb.h"
#include "layouts/kb_layouts.h"
#include "mem.h"
#include "ports.h"
#include <drivers/uhci.h>
#include <stdint.h>
#include <terminal/printf.h>

// This is only the boot protocol, report protocol will be for later

void SetProtocol(struct USBDevice usb, uint16_t protocol) {
    if (protocol == REPORT_PROTOCOL) return; // Only boot protocol
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
}

struct KeyboardReport* HIDKeyboardInit(struct USBDevice* usb) {
    struct KeyboardReport* report = kmalloc(sizeof(struct KeyboardReport));
    memset(report, 0, sizeof(struct KeyboardReport));

    usb->InitInterruptTranfers(&usb->data);
    usb->SetInterruptTransfer(&usb->data, usb->data.endpoint[0].bInterval, report, sizeof(*report));

    usb->data.user_data = report;

    return report;
}

// only one keyboard supported because of this
bool hid_kb_ready = 0;

void ManageKeyboardReport(struct USBDevice device) {
    struct KeyboardReport* report = device.data.user_data;

    KEYSTATE.CtrlL = report->modifier_key_status & (1 << 0);
    KEYSTATE.ShiftL = report->modifier_key_status & (1 << 1);
    KEYSTATE.AltL = report->modifier_key_status & (1 << 2);
    KEYSTATE.CtrlR = report->modifier_key_status & (1 << 4);
    KEYSTATE.ShiftR = report->modifier_key_status & (1 << 5);
    KEYSTATE.AltR = report->modifier_key_status & (1 << 6);

    for (int i = 0; i < sizeof(report->keypresses); i++) {
        if (!report->keypresses[i]) continue;
        else {
            set_layout(HID_LAYOUTS[0]);
            last_scancode = report->keypresses[i];
            hid_kb_ready = 1;
            break;
        }
    }
}
scancode_t hid_wfi() {
    while (true) {
        if (hid_kb_ready) {
            hid_kb_ready = 0;
            return last_scancode;
        }
        HALT();
    } return 0;
}