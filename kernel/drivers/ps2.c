#include "drivers/ps2.h"
#include "drivers/pit.h"
#include <ports.h>
#include <stdbool.h>
#include <stdint.h>
#include <terminal/printf.h>

/*
    If the machine running this dosen't have a ps2 controller, enabling ps2 may cause a system crash.
    To know if the machine has one, we need to parse the aml from the acpi
*/

/*
    Someone keep working on this, it dosen't work but it do something at least
*/

void ps2_clear_input_buffer_status() {
    uint8_t ps2_sts = inb(PS2_CMD_PORT);
    if (ps2_sts & (1 << 1)) // If the Input buffer status is set (full), clear it
        outb(PS2_CMD_PORT, ps2_sts & ~(1 << 1));
    inb(PS2_DATA_PORT);
}

bool test_ps2_controller() {
    ps2_clear_input_buffer_status();
    
    outb(PS2_CMD_PORT, 0xAA);
    return inb(PS2_DATA_PORT) == 0x55 /* Test passed */ ? true : false;
}

bool test_ps2_port(bool port) {
    ps2_clear_input_buffer_status();

    outb(PS2_CMD_PORT, port ? 0xA9 : 0xAB); // Check the first port or the second one
    return inb(PS2_DATA_PORT) == 0x00 /* Port test passed */ ? true : false;
}

void disable_ps2_configuration_bytes(uint8_t bytes) {
    ps2_clear_input_buffer_status();

    outb(PS2_CMD_PORT, 0x20);
    uint8_t config_byte = inb(PS2_DATA_PORT);

    outb(PS2_CMD_PORT, 0x60);
    outb(PS2_CMD_PORT, config_byte & ~bytes);
}
void enable_ps2_configuration_bytes(uint8_t bytes) {
    ps2_clear_input_buffer_status();

    outb(PS2_CMD_PORT, 0x20);
    uint8_t config_byte = inb(PS2_DATA_PORT);

    outb(PS2_CMD_PORT, 0x60);
    outb(PS2_CMD_PORT, config_byte | bytes);
}

uint8_t read_ps2_configuration_byte() {
    ps2_clear_input_buffer_status();

    outb(PS2_CMD_PORT, 0x20);
    return inb(PS2_DATA_PORT);
}

void disable_ps2_devices() {
    ps2_clear_input_buffer_status();

    outb(PS2_CMD_PORT, 0xAD);
    outb(PS2_CMD_PORT, 0xA7);
}

void enable_ps2_devices() {
    ps2_clear_input_buffer_status();

    outb(PS2_CMD_PORT, 0xAE); // Enable first port
    outb(PS2_CMD_PORT, 0xA8); // Enable second port

    // enable_ps2_configuration_bytes((1 << 0) | (1 << 1) | (1 << 6));
}

bool ps2_wait(bool type) {
    int timeout = 500;
    if (!type) while (((inb(PS2_CMD_PORT) & 1) != 1) && --timeout) pit_timer_wait_ms(1);
    else while (((inb(PS2_CMD_PORT) & 2) != 0) && --timeout) pit_timer_wait_ms(1);
    return timeout;
}

uint8_t send_ps2_device_byte(uint8_t byte, bool device) {
    if (device) {
        if (ps2_wait(true)) return 0x01; // Timeout
        outb(PS2_CMD_PORT, 0xD4);
    }

    // Note: Check if the ps2 controller is dual channel before sending bytes to the second device

    if (ps2_wait(true)) return 0x01;
    outb(PS2_DATA_PORT, byte);
    if (ps2_wait(false)) return 0x01;

    return inb(PS2_DATA_PORT);
}

uint8_t ps2_init() {
    uint8_t ret_code = 0;

    disable_ps2_devices();

    if (!(inb(PS2_CMD_PORT) & (1 << 2))) /* This bit is set, the bios didn't pass the POST */ {
        printf("How are you here?\n");
        return 0xFF;
    }

    inb(PS2_DATA_PORT); // Discard useless info
    disable_ps2_configuration_bytes((1 << 0) | (1 << 1) | (1 << 6) | (1 << 4)); // disable interruptions

    if (!test_ps2_controller()) return ERRNO_CONTROLLER_TEST;
    
    // Check if it is dual channel (2 ports)
    outb(PS2_CMD_PORT, 0xA8);
    bool is_dual_channel = !(read_ps2_configuration_byte() & (1 << 5));

    if (is_dual_channel) {
        outb(PS2_CMD_PORT, 0xA7);
        disable_ps2_configuration_bytes((1 << 1) | (1 << 5));
        ret_code |= (1 << 1);
    }

    // Checks the 2 ports
    outb(PS2_CMD_PORT, 0xAB);
    if (inb(PS2_DATA_PORT) != 0) ret_code |= (1 << 2);

    if (is_dual_channel) {
        outb(PS2_CMD_PORT, 0xA9);
        if (inb(PS2_DATA_PORT) != 0) ret_code |= (1 << 3);
    }

    enable_ps2_devices();
    enable_ps2_configuration_bytes((1 << 0) | (1 << 1));

    // Restart both of the devices

    if (send_ps2_device_byte(0xF5, 0) != 0xFA) {
        // printf("Eror see\n");
    }


    return ret_code;
}