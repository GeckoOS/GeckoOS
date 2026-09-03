#pragma once

#include <stdbool.h>
#include "drivers/usb.h"

#define BOOT_PROTOCOL 0
#define REPORT_PROTOCOL 1

void SetProtocol(struct USBDevice usb, uint16_t protocol);
void GetReport(struct USBDevice usb);