#pragma once

#include <stdbool.h>
#include <stdint.h>

#define PS2_CMD_PORT 0x64 // read/write PS/2 command register
#define PS2_DATA_PORT                                                        \
    0x60 // PS/2 data port register see
         // https://wiki.osdev.org/%228042%22_PS/2_Controller

#define ERRNO_CONTROLLER_TEST 1

bool test_ps2_controller();
bool test_ps2_port(bool port);
void disable_ps2_devices();
void enable_ps2_devices();
uint8_t ps2_init();