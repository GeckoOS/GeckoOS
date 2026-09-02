//going to hate my life here we go

// damn like fr

#include "drivers/uhci.h"
#include "drivers/pci.h"
#include "drivers/pit.h"
#include "drivers/tables/irq.h"
#include "drivers/tables/isr.h"
#include "drivers/usb.h"
#include "gk/gk.h"
#include "mem.h"
#include "ports.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <terminal/printf.h>
#include "drivers/vga.h"
#include <mem/paging.h>

void SetUHCIRegisterW(struct UHCIDevice controller, uint8_t reg, uint16_t data) {
    outw(controller.ioport + reg, data);
}
void SetUHCIRegisterL(struct UHCIDevice controller, uint8_t reg, uint32_t data) {
    outl(controller.ioport + reg, data);
}
uint16_t ReadUHCIRegisterW(struct UHCIDevice controller, uint8_t reg) {
    return inw(controller.ioport + reg);
}
uint32_t ReadUHCIRegisterL(struct UHCIDevice controller, uint8_t reg) {
    return inl(controller.ioport + reg);
}

static bool SendUHCIPacket(struct BasicUSBHeader* header, struct usb_setup_packet setup_packet, void* buffer, bool twice);

void uhci_interrupt(registers_t* r) {
    printf("\nERR: UHCI Timeout/Crc!\n");
}

void SetFrameEntry(struct UHCIDevice* controller, uint32_t index, uint32_t entry, bool framedisable, bool type) {
    if (!controller->framelist) return;

    controller->framelist[index] = (entry & ~0xF) |
        (framedisable << 0) |
        (type << 1);
}

uint32_t GetQueueHeadEntry(uint32_t entry, bool framedisable, bool type) {
    return (entry & ~0xF) |
        (framedisable << 0) |
        (type << 1);
}

struct USBDevice* uhci_init(struct PCIDevice device) {
    IOBar bar4 = PCIGetIOBar(device, 4);

    struct UHCIDevice* controller = kmalloc(sizeof(struct UHCIDevice));
    controller->framelist = (FrameEntry*)ALIGN((uint64_t)kmalloc_align(1024 * sizeof(FrameEntry), 4096), 4096);
    controller->tdpool = (struct UHCITransferDescriptor*)ALIGN((uint64_t)kmalloc_align(32 * sizeof(struct UHCITransferDescriptor), 16), 16);
    controller->qhpool = (struct UHCIQueueHead*)ALIGN((uint64_t)kmalloc_align(16 * sizeof(struct UHCIQueueHead), 16), 16);
    memset(controller->framelist, 0, 1024 * sizeof(FrameEntry));

    if (!bar4) {
        set_printf_color(VGA_COLOR_LIGHT_RED);
            printf("  UHCI Controller is invalid!\n");
        set_printf_color(VGA_COLOR_WHITE);
        goto err;
    }

    uint16_t ioport = bar4 & ~0x3;
    controller->ioport = ioport;

    // Take control from BIOS
    pci_writew(device.bus, device.slot, device.func, 0xC0, 0x2000);

    // Enable I/O Bus mastering
    pci_writew(device.bus, device.slot, device.func, 0x4, pci_readw(device.bus, device.slot, device.func, 0x4) | (1 << 2) | (1 << 0));

    // Enable interrupts (Timeout CRC)
    SetUHCIRegisterW(*controller, USBINTR, INTR_TIE);

    // Host controller reset
    SetUHCIRegisterW(*controller, USBCMD, ReadUHCIRegisterW(*controller, USBCMD) | CMD_HCRESET);
    pit_timer_wait_ms(25);

    int timeout = 1000;
    while ((ReadUHCIRegisterW(*controller, USBCMD) & CMD_HCRESET) && --timeout) pit_timer_wait_ms(1);
    if (!timeout) {
        printc("Failed to restart UHCI Controller\n", VGA_COLOR_LIGHT_RED);
        goto err;
    }

    // Set Frame List Base Address
    SetUHCIRegisterL(*controller, FRBASE, (uint32_t)(controller->framelist) & ~0xFFF);
    SetUHCIRegisterW(*controller, FRNUM, 0x0);
    SetUHCIRegisterW(*controller, SOFMOD, 0x40);

    // Clean the status register
    SetUHCIRegisterW(*controller, USBSTS, 0xFFFF);

    // Enable it
    SetUHCIRegisterW(*controller, USBCMD, CMD_RS | CMD_MAXP);
    pit_timer_wait_ms(5);
    
    if (ReadUHCIRegisterW(*controller, USBSTS) & STS_HCHALT) {
        printf("UHCI error when enabling it!");
        goto err;
    }

    struct USBDevice* devices = kmalloc(sizeof(struct USBDevice*) * 2);
    memset(devices, 0, sizeof(struct USBDevice) * 2);

    // Look if a usb is connected in any of the 2 ports
    for (int i = 0; i < 2; i++) {
        uint8_t reg = PORTSC1 + (i * 2);

        uint16_t port = ReadUHCIRegisterW(*controller, reg);
        if (port & 1 && port != 0xFFFF) {
            // Enable reset
            uint16_t cmd = (port & ~((1 << 1) | (1 << 3))) | (1 << 9);
            SetUHCIRegisterW(*controller, reg, cmd);
            pit_timer_wait_ms(50);

            // Disable reset
            cmd &= ~(1 << 9);
            SetUHCIRegisterW(*controller, reg, cmd);
            pit_timer_wait_ms(10);

            SetUHCIRegisterW(*controller, reg, (ReadUHCIRegisterW(*controller, reg) & ~0x13) | (1 << 1) | (1 << 3));

            // Enable port
            SetUHCIRegisterW(*controller, reg, (1 << 2) | (1 << 1) | (1 << 3));

            int timeout = 100;
            while (!(ReadUHCIRegisterW(*controller, reg) & (1 << 2)) && --timeout) pit_timer_wait_ms(1);
            if (!timeout) {
                set_printf_color(VGA_COLOR_LIGHT_RED);
                    printf("Failed to enable UHCI Controller's port %d\n", i + 1);
                set_printf_color(VGA_COLOR_WHITE);
                continue;
            }

            devices[i].data.lowspeed = ReadUHCIRegisterW(*controller, reg) & (1 << 8);
            devices[i].type = UHCICONTROLLER;
            devices[i].data.device_address = 0;
            devices[i].data.controller = controller;

            if (!SendUHCIPacket(&devices[i].data, (struct usb_setup_packet){
                    .requesttype = 0x80,
                    .request = REQUEST_GET_DESCRIPTOR,
                    .value = DESCRIPTOR_TYPE_DEVICE << 8,
                    .index = 0,
                    .lenght = sizeof(struct usb_device_descriptor) - 1
                }, &devices[i].data.device_descriptor, false)) {
                    printf("UHCI Controller port %d: Failed to get device descriptor\n", i);
                    devices[i].data.controller = NULL;
                    continue;
                }
            if (!SendUHCIPacket(&devices[i].data, (struct usb_setup_packet){
                    .requesttype = 0x00,
                    .request = REQUEST_SET_ADDRESS,
                    .value = i + 1,
                    .index = 0x00,
                    .lenght = 0x00
                }, 0, true)) {
                    devices[i].data.controller = NULL;
                    printf("UHCI Controller port %d: Failed to set address\n", i);
                    continue;
                }

            devices[i].data.device_address = i + 1;
            devices[i].data.is_hid = IsHID(devices[i]);
            devices[i].SendPacket = SendUHCIPacket;

            // Set every usb to its own device address, and split them into differents USB devices from the same controller
        }
    }

    // irq_install_handler(usb.irq, uhci_interrupt, 0);

    return devices;
err:
    kfree(controller->framelist);
    controller->framelist = NULL;
    kfree(controller->tdpool);
    kfree(controller->qhpool);
    kfree(controller);
    return NULL;
}

#ifdef DEBUG
    void DumpTd(struct UHCITransferDescriptor td) {
        printf("TD Dump:\n");
        printf("  TD Buffer: %x\n", td.buffer);
        printf("  TD Next:\n");
        printf("    Physical address: %x\n", td.next & ~0xF);
        printf("    Depth: %x\n", td.next & (1 << 2));
        printf("    Memory structure type: %s\n", td.next & (1 << 1) ? "Queue head" : "Transfer descriptor");
        printf("    Terminate: %x\n", td.next & (1 << 0));
        printf("  TD Status:\n");
        if (td.status & (1 << 17)) printf("    Bit stuff error\n");
        if (td.status & (1 << 18)) printf("    Timeout CRC\n");
        if (td.status & (1 << 19)) printf("    Non-Acknowledged\n");
        if (td.status & (1 << 20)) printf("    Babble Detected\n");
        if (td.status & (1 << 21)) printf("    Data Buffer Error\n");
        if (td.status & (1 << 22)) printf("    Stalled\n");
        if (td.status & (1 << 23)) printf("    Active\n");
        if (td.status & (1 << 24)) printf("    Interrupt On Complete\n");
        if (td.status & (1 << 25)) printf("    Is Isochronous\n");
        if (td.status & (1 << 26)) printf("    Low speed\n");
        if (td.status & (1 << 29)) printf("    Short Packet Detect\n");
        printf("    Error counter: %x\n", (td.status >> 17) & 0x1F);
        printf("    Actual length: %x\n", td.status & 0x7FF);
        printf("  TD Packet header:\n");
        printf("    Maximum length: %x\n", td.packet_header >> 21);
        printf("    Data toggle: %x\n", td.packet_header & (1 << 19));
        printf("    Packet type: %x\n", td.packet_header & 0xFF);
        printf("    Endpoint: %x\n", (td.packet_header >> 15) & 0x7);
        printf("    Device: %x\n", (td.packet_header >> 8) & 0x3F);
    }
#endif

static bool GetUHCIDescriptor(struct BasicUSBHeader* uhci, struct usb_setup_packet setup, void* buffer, bool wait /* Waits for the packets to be readed */, bool twice) {
    struct UHCIDevice* controller = (struct UHCIDevice*)uhci->controller;

    // TD 0
    controller->tdpool[0].buffer = (uint32_t)&setup;
    controller->tdpool[0].next = ((uint32_t)(&controller->tdpool[1]) & ~0xF) | (1 << 2);
    controller->tdpool[0].packet_header = (HEADER_TYPE_SETUP & 0xFF) | (uhci->device_address << 8) | ((0x7 & 0x7ff) << 21);
    controller->tdpool[0].status = 0 | (1 << 23) | (uhci->lowspeed << 26) | (3 << 27);

    // TD 1
    controller->tdpool[1].buffer = (uint32_t)buffer;
    controller->tdpool[1].next = twice ? 1 : ((uint32_t)(&controller->tdpool[2]) & ~0xF) | (1 << 2);
    controller->tdpool[1].packet_header = (HEADER_TYPE_IN & 0xFF) | (uhci->device_address << 8) | (1 << 19) | (((setup.lenght - 1) & 0x7ff) << 21);
    controller->tdpool[1].status = (1 << 23) | (uhci->lowspeed << 26) | (3 << 27) | (1 << 29);

    // TD 2
    controller->tdpool[2].buffer = 0;
    controller->tdpool[2].next = 1;
    controller->tdpool[2].packet_header = (HEADER_TYPE_OUT & 0xFF) | (uhci->device_address << 8) | (1 << 19) | ((0 & 0x7ff) << 21);
    controller->tdpool[2].status = (1 << 23) | (uhci->lowspeed << 26) | (3 << 27);

    controller->qhpool[0].vertical_pointer = GetQueueHeadEntry((uint32_t)&controller->tdpool[0], 0, FRAME_TYPE_TD);
    controller->qhpool[0].horizontal_pointer = GetQueueHeadEntry(0, 1, 0);
    SetFrameEntry(controller, 0, (uint32_t)&controller->qhpool[0], 0, FRAME_TYPE_QD);

    if (wait)
        for (int i = 0; i < 3 - twice; i++) {
            #ifdef DEBUG
                printf(".");
            #endif
            while (controller->tdpool[i].status & (1 << 23)) PAUSE(); // Wait for the TD active bit to turn off

            // Check for errors
            if (controller->tdpool[i].status & ((1 << 22) | (1 << 21) | (1 << 20) | (1 << 19) | (1 << 18) | (1 << 17))) {
                #ifdef DEBUG
                    DumpTd(controller->tdpool[i]);
                    printf("TD%d Failed!\n", i + 1);
                #endif
                return false;
            }
    } SetUHCIRegisterW(*controller, USBSTS, 0x00FF);

    return true;
}

static bool SendUHCIPacket(struct BasicUSBHeader* header, struct usb_setup_packet setup_packet, void* buffer, bool twice) { return GetUHCIDescriptor(header, setup_packet, buffer, true, twice); }

void uhci_free(struct UHCIDevice* controller) {
    kfree(controller->framelist);
    kfree(controller->tdpool);
    kfree(controller->qhpool);
    kfree(controller);
}