#pragma once

#include <stdbool.h>
#include "drivers/usb.h"
#include "layouts/kb_layouts.h"

#define BOOT_PROTOCOL 0
#define REPORT_PROTOCOL 1 // TODO: Make a separate file for all the HID things

struct KeyboardReport {
    uint8_t modifier_key_status;
    uint8_t reserved;
    uint8_t keypresses[6];
} __attribute__((packed));

void SetProtocol(struct USBDevice usb, uint16_t protocol);
void GetReport(struct USBDevice usb);
struct KeyboardReport* HIDKeyboardInit(struct USBDevice* usb);
void ManageKeyboardReport(struct USBDevice device);
scancode_t hid_wfi();