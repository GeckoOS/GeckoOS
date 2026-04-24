#ifndef _BOOTOPSTIONS_H
#define _BOOTOPSTIONS_H

#include "ports.h"

#define reboot() outb(0x64, 0xfe);

// QEMU
#define QEMU_poweroff() outw(0x604, 0x2000); /* Only for debugging (QEMU only) */
#define QEMU2_0_poweroff() outw(0xB004, 0x2000); /* Only for debugging (For olders version of QEMU (than 2.0)) */

// VirtualBox poweroff
#define Vb_poweroff() outw(0x4004, 0x3400);

// Cloud Hypervisor poweroff
#define Ch_poweroff() outw(0x600, 0x34);

#endif
