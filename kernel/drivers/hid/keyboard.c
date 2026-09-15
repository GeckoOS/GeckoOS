#include "drivers/hid/keyboard.h"
#include "drivers/input.h"
#include "drivers/ps2keyboard.h"
#include "drivers/usb.h"
#include "layouts/kb_layouts.h"
#include "mem.h"
#include <drivers/interfaces/uhci.h>
#include <stdint.h>
#include <terminal/printf.h>
#include <drivers/pit.h>

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
    if (!usb->data.user_data) {
        usb->data.user_data = kmalloc(sizeof(struct KeyboardReport));
        memset(usb->data.user_data, 0, sizeof(struct KeyboardReport));
    }

    usb->InitInterruptTranfers(&usb->data);
    usb->SetInterruptTransfer(&usb->data, INTERRUPT_TRANSFER_INTERVAL_8MS, usb->data.user_data, sizeof(struct KeyboardReport));

    // actual_input = 1; // set to hid output

    return usb->data.user_data;
}

// only one keyboard supported because of this
bool hid_kbs_ready[16];

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
            #ifdef DEBUG
                if (report->keypresses[i] <= 0x4) {
                    switch (report->keypresses[i]) {
                        case 0x1: printf("\nhid_kbd: phantom scancode\n");
                        case 0x2: printf("\nhid_kbd: self-test failed\n");
                        case 0x3: printf("\nhid_kbd: undefined error\n");
                    }
                }
            #endif

            set_layout(HID_LAYOUTS[0]);
            actual_input = 1; // usb keyboard
            hid_kbs_ready[device.index] = 1;
            last_scancode = report->keypresses[i];
            break;
        }
    }
}
scancode_t hid_wfi() {
    for (int i = 0; i < 16; i++) {
        if (hid_kbs_ready[i]) {
            hid_kbs_ready[i] = 0;
            return last_scancode;
        }

        pit_timer_wait_ms(5);
    } return 0;
}