
#include "drivers/mouse.h"
#include "drivers/tables/irq.h"
#include "drivers/vga.h"
#include "ports.h"
#include "sys/types.h"
#include "terminal/printf.h"
#include "terminal/terminal.h"
#include <mem.h>
#include <stdbool.h>
#include <stdint.h>

int g_mouse_x_pos = 0, g_mouse_y_pos = 0;
MOUSE_STATUS g_status;

int mouse_getx() { return g_mouse_x_pos; }

int mouse_gety() { return g_mouse_y_pos; }

void mouse_wait(bool type)
{
    uint32_t time_out = 100000;
    if (type == false) {
        // suspend until status is 1
        while (time_out--) {
            if ((inb(PS2_CMD_PORT) & 1) == 1) {
                return;
            }
        }
        return;
    } else {
        while (time_out--) {
            if ((inb(PS2_CMD_PORT) & 2) == 0) {
                return;
            }
        }
    }
}

void mouse_write(uint8_t data)
{
    // sending write command
    mouse_wait(true);
    outb(PS2_CMD_PORT, 0xD4);
    mouse_wait(true);
    // finally write data to port
    outb(MOUSE_DATA_PORT, data);
}

uint8_t mouse_read()
{
    mouse_wait(false);
    return inb(MOUSE_DATA_PORT);
}

void get_mouse_status(char status_byte, MOUSE_STATUS *status)
{
    memset(status, 0, sizeof(MOUSE_STATUS));
    if (status_byte & 0x01)
        status->left_button = 1;
    if (status_byte & 0x02)
        status->right_button = 1;
    if (status_byte & 0x04)
        status->middle_button = 1;
    if (status_byte & 0x08)
        status->always_1 = 1;
    if (status_byte & 0x10)
        status->x_sign = 1;
    if (status_byte & 0x20)
        status->y_sign = 1;
    if (status_byte & 0x40)
        status->x_overflow = 1;
    if (status_byte & 0x80)
        status->y_overflow = 1;
}

void print_mouse_info()
{
    // console_gotoxy(0, 0);
    printf("Mouse Demo X: %d, Y: %d\n", g_mouse_x_pos, g_mouse_y_pos);
    if (g_status.left_button) {
        printf("Left button clicked");
    }
    if (g_status.right_button) {
        printf("Right button clicked");
    }
    if (g_status.middle_button) {
        printf("Middle button clicked");
    }
}

void mouse_handler(registers_t *r)
{
    static uint8_t mouse_cycle = 0;
    static char mouse_byte[4];

    switch (mouse_cycle) {
    case 0:
        mouse_byte[0] = mouse_read(); // get buttons
        get_mouse_status(mouse_byte[0], &g_status);
        mouse_cycle++;

        break;
    case 1:
        mouse_byte[1] = mouse_read();
        mouse_cycle++;
        break;
    case 2:
        mouse_byte[2] = mouse_read();
        g_mouse_x_pos =
            g_mouse_x_pos +
            mouse_byte[1]; //*0.15;//sensibility  modifided from posistion moved
                           // here otherwise strange things happen
        g_mouse_y_pos = g_mouse_y_pos - mouse_byte[2]; //*0.15;

        if (g_mouse_x_pos < 0)
            g_mouse_x_pos = 0;
        if (g_mouse_y_pos < 0)
            g_mouse_y_pos = 0;
        if (g_mouse_x_pos > VGA_TEXT_WIDTH)
            g_mouse_x_pos = VGA_TEXT_WIDTH - 1;
        if (g_mouse_y_pos > VGA_TEXT_HEIGHT)
            g_mouse_y_pos = VGA_TEXT_HEIGHT - 1;

        // terminal_clear(VGA_COLOR_BLACK);
        // (g_mouse_x_pos, g_mouse_y_pos);
        // putchar('X',VGA_COLOR_LIGHT_GREEN);
        // print_mouse_info();
        mouse_cycle = 0;
        break;
    }
    // store data a write a callback so that we can overwrite the behaviour of
    // the handler without modifying IDT
    mouse_event_data_t md;
    // it s a bitmask with buttons can be simplified witha  high level handler
    // something like that is above
    md.buttons    = mouse_byte[0];
    md.event_type = 0; // Move mouse
    md.x          = g_mouse_x_pos;
    md.y          = g_mouse_y_pos;
    md.scroll     = (int8_t)mouse_byte[3];
    if (mouse_event_run != NULL) {
        // Call the registered callback function
        mouse_event_run(md);
    } else {
        printf("No callback registered\n");
    }
}

/**
 * available rates 10, 20, 40, 60, 80, 100, 200
 */
void set_mouse_rate(uint8_t rate)
{
    uint8_t status;

    outb(MOUSE_DATA_PORT, MOUSE_CMD_SAMPLE_RATE);
    status = mouse_read();
    if (status != MOUSE_ACKNOWLEDGE) {
        printf("error: failed to send mouse sample rate command\n");
        return;
    }
    outb(MOUSE_DATA_PORT, rate);
    status = mouse_read();
    if (status != MOUSE_ACKNOWLEDGE) {
        printf("error: failed to send mouse sample rate data\n");
        return;
    }
}
// static uint8_t mouse_read_id(){

//  mouse_wait(true);
//     outb(0x64,0xD4);
//     outb(MOUSE_DATA_PORT, 0xF2);
//     mouse_wait(1);
//     mouse_read();


//     return mouse_read()
// }
void mouse_init()
{
    uint8_t status;

    g_mouse_x_pos = 5;
    g_mouse_y_pos = 2;

    printf("initializing mouse...\n");

    // enable mouse device
    mouse_wait(true);
    outb(PS2_CMD_PORT, 0xA8);

    // outb(MOUSE_DATA_PORT,0xF5);

    // print mouse id

    outb(MOUSE_DATA_PORT, MOUSE_CMD_MOUSE_ID);
    status = mouse_read();
    printf("mouse id: 0x%x\n", status);
    // mouse id will change if it has more buttons after  sending some random
    //* packages for some reasone the sequence for scroll whell is 200 100 80
    // and
    // for buttons it is 200 200 80

    // magic sequence hate this shit
    set_mouse_rate(200);

    set_mouse_rate(100);


    set_mouse_rate(80);


    outb(MOUSE_DATA_PORT, MOUSE_CMD_MOUSE_ID);
    mouse_wait(true);
    status = mouse_read();
    if (status == 3) {
        printf("Mouse has accepted it has whell %d\n", status);
    } else {
        printf("Mouse renounces his whell %d\n", status);
    }
    //* activating buttons
    set_mouse_rate(200);
    mouse_wait(true);

    set_mouse_rate(200);
    mouse_wait(true);

    set_mouse_rate(80);
   
    if (status == 4) {
        printf("Mouse has accepted it has buttons %d\n", status);
    } else {
        printf("Mouse renounces his buttons %d\n", status);
    }
    status = mouse_read();
    // outb(MOUSE_DATA_PORT, MOUSE_CMD_RESOLUTION);
    // outb(MOUSE_DATA_PORT, 0);

    // enable the interrupt
    mouse_wait(true);
    outb(PS2_CMD_PORT, 0x20);
    mouse_wait(false);
    // get and set second bit
    status = (inb(MOUSE_DATA_PORT) | 2);
    // write status to port
    mouse_wait(true);
    outb(PS2_CMD_PORT, MOUSE_DATA_PORT);
    mouse_wait(true);
    outb(MOUSE_DATA_PORT, status);

    // set mouse to use default settings
    mouse_write(MOUSE_CMD_SET_DEFAULTS);
    status = mouse_read();
    if (status != MOUSE_ACKNOWLEDGE) {
        printf("error: failed to set default mouse settings\n");
        return;
    }

    // enable packet streaming to receive
    mouse_write(MOUSE_CMD_ENABLE_PACKET_STREAMING);
    status = mouse_read();
    if (status != MOUSE_ACKNOWLEDGE) {
        printf("error: failed to enable mouse packet streaming\n");
        return;
    }

    // set mouse handler
    irq_install_handler(12, mouse_handler);
}

void register_mouse_callback(mouse_event_callback callback)
{ mouse_event_run = callback; }

void simulate_mouse_event(mouse_event_data_t md)
{
    if (mouse_event_run != NULL) {
        // Call the registered callback function
        mouse_event_run(md);
    } else {
        printf("No mouse callback registered\n");
    }
}