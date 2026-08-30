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
    outw(controller.header.ioport + reg, data);
}
void SetUHCIRegisterL(struct UHCIDevice controller, uint8_t reg, uint32_t data) {
    outl(controller.header.ioport + reg, data);
}
uint16_t ReadUHCIRegisterW(struct UHCIDevice controller, uint8_t reg) {
    return inw(controller.header.ioport + reg);
}
uint32_t ReadUHCIRegisterL(struct UHCIDevice controller, uint8_t reg) {
    return inl(controller.header.ioport + reg);
}

void UHCIHandler(struct BasicUSBHeader* header) {
    struct UHCIDevice* controller = (struct UHCIDevice*)header;

    // Does nothing...
}

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

struct USBDevice uhci_init(struct PCIDevice device) {
    IOBar bar4 = PCIGetIOBar(device, 4);

    struct UHCIDevice* controller = kmalloc(sizeof(struct UHCIDevice));
    controller->header.device = device;
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
    controller->header.ioport = ioport;

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
    }

    // Set Frame List Base Address
    SetUHCIRegisterL(*controller, FRBASE, (uint32_t)(controller->framelist) & ~0xFFF);
    SetUHCIRegisterW(*controller, FRNUM, 0x0);
    SetUHCIRegisterW(*controller, SOFMOD, 0x40);

    // Clean the status register
    SetUHCIRegisterW(*controller, USBSTS, 0xFFFF);

    // Find ports
    for (int i = 0; i < 2; i++) {
        uint8_t reg = PORTSC1 + (i * 2);

        uint16_t port = ReadUHCIRegisterW(*controller, reg);
        if (port & 1) {
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
            }
            controller->speed = ReadUHCIRegisterW(*controller, reg) & (1 << 8);
            #ifdef DEBUG
                uint16_t final_port = ReadUHCIRegisterW(*controller, reg);

                int is_low_speed = (final_port & (1 << 8)) ? 1 : 0;
                printf("Port %d enabled successfully! Speed: %s\n", 
                    i + 1, is_low_speed ? "Low-Speed (1.5Mbps)" : "Full-Speed (12Mbps)");
            #endif
        }
    }

    // Enable it
    SetUHCIRegisterW(*controller, USBCMD, CMD_RS | CMD_MAXP);
    pit_timer_wait_ms(5);
    
    if (ReadUHCIRegisterW(*controller, USBSTS) & STS_HCHALT) {
        printf("UHCI error when enabling it!");
        goto err;
    }

    controller->address = 0;

    struct USBDevice usb;
    usb.data = controller;
    usb.type = UHCICONTROLLER;
    usb.handler = UHCIHandler;
    usb.irq = pci_readb(device.bus, device.slot, device.func, 0x3c);

    irq_install_handler(usb.irq, uhci_interrupt, 0);

    return usb;
err:
    kfree(controller->framelist);
    controller->framelist = NULL;
    kfree(controller->tdpool);
    kfree(controller->qhpool);
    kfree(controller);
    return (struct USBDevice){0};
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

void GetUHCIDescriptor(struct UHCIDevice* controller, struct usb_setup_packet setup, void* buffer, bool wait /* Waits for the packets to be readed */) {
    // TD 0
    controller->tdpool[0].buffer = (uint32_t)&setup;
    controller->tdpool[0].next = ((uint32_t)(&controller->tdpool[1]) & ~0xF) | (1 << 2);
    controller->tdpool[0].packet_header = (HEADER_TYPE_SETUP & 0xFF) | (controller->address << 8) | ((0x7 & 0x7ff) << 21);
    controller->tdpool[0].status = 0 | (1 << 23) | (controller->speed << 26) | (3 << 27);

    // TD 1
    controller->tdpool[1].buffer = (uint32_t)buffer;
    controller->tdpool[1].next = ((uint32_t)(&controller->tdpool[2]) & ~0xF) | (1 << 2);
    controller->tdpool[1].packet_header = (HEADER_TYPE_IN & 0xFF) | (controller->address << 8) | (1 << 19) | (((setup.lenght - 1) & 0x7ff) << 21);
    controller->tdpool[1].status = (1 << 23) | (controller->speed << 26) | (3 << 27) | (1 << 29);

    // TD 2
    controller->tdpool[2].buffer = 0;
    controller->tdpool[2].next = 1;
    controller->tdpool[2].packet_header = (HEADER_TYPE_OUT & 0xFF) | (controller->address << 8) | (1 << 19) | ((0 & 0x7ff) << 21);
    controller->tdpool[2].status = (1 << 23) | (controller->speed << 26) | (3 << 27);

    controller->qhpool[0].vertical_pointer = GetQueueHeadEntry((uint32_t)&controller->tdpool[0], 0, FRAME_TYPE_TD);
    controller->qhpool[0].horizontal_pointer = GetQueueHeadEntry(0, 1, 0);
    SetFrameEntry(controller, 0, (uint32_t)&controller->qhpool[0], 0, FRAME_TYPE_QD);

    if (wait)
        for (int i = 0; i < 3; i++) {
            #ifdef DEBUG
                printf("Waiting for packet %d (0x%p) to be readed...\n", i + 1, &controller->tdpool[i]);
            #endif
            while (controller->tdpool[i].status & (1 << 23)) PAUSE(); // Wait for the TD active bit to turn off

            // Check for errors
            if (controller->tdpool[i].status & ((1 << 22) | (1 << 21) | (1 << 20) | (1 << 19) | (1 << 18) | (1 << 17))) {
                #ifdef DEBUG
                    DumpTd(controller->tdpool[i]);
                #endif
                printf("TD%d Failed!\n", i + 1);
                break;
            }
    } SetUHCIRegisterW(*controller, USBSTS, 0x00FF);
}

void SetUHCIDeviceAddress(struct UHCIDevice* controller, uint8_t address) {
    struct usb_setup_packet packet;
    packet.requesttype = 0x00;
    packet.request = 0x05;
    packet.value = address;
    packet.index = 0x00;
    packet.lenght = 0x00;

    if (address > 127) {
        printc("WARNING", VGA_COLOR_YELLOW);
        printc(": The UHCI Controller address is greater than the limit (127) and may cause the behavior of the device to be undefined\n", VGA_COLOR_YELLOW);
    } if (!address) return;

    // TD 0 (SETUP)
    controller->tdpool[0].buffer = (uint32_t)&packet;
    controller->tdpool[0].next = ((uint32_t)(&controller->tdpool[1]) & ~0xF) | (1 << 2);
    controller->tdpool[0].packet_header = (HEADER_TYPE_SETUP & 0xFF) | (controller->address << 8) | ((0x7 & 0x7ff) << 21);
    controller->tdpool[0].status = (1 << 23) | (controller->speed << 26) | (3 << 27);

    // TD 1 (STATUS)
    controller->tdpool[1].buffer = 0;
    controller->tdpool[1].next = 1;
    controller->tdpool[1].packet_header = (HEADER_TYPE_IN & 0xFF) | (controller->address << 8) | (1 << 19) | ((0x7ff & 0x7ff) << 21);

    controller->tdpool[1].status = (1 << 23) | (controller->speed << 26) | (3 << 27);

    controller->qhpool[0].vertical_pointer = GetQueueHeadEntry((uint32_t)(&controller->tdpool[0]), 0, FRAME_TYPE_TD);
    controller->qhpool[0].horizontal_pointer = GetQueueHeadEntry(0, 1, 0);
    SetFrameEntry(controller, 0, (uint32_t)(&controller->qhpool[0]), 0, FRAME_TYPE_QD);

    for (int i = 0; i < 2; i++) {
        #ifdef DEBUG
            printf("Waiting for packet %d (0x%p) to be readed...\n", i + 1, &controller->tdpool[i]);
        #endif
        while (controller->tdpool[i].status & (1 << 23)) PAUSE();

        // Check for errors
        if (controller->tdpool[i].status & ((1 << 22) | (1 << 21) | (1 << 20) | (1 << 18) | (1 << 17))) {
            #ifdef DEBUG
                DumpTd(controller->tdpool[i]);
            #endif
            printf("TD%d Failed! Status\n", i + 1);
            return;
        }
    } pit_timer_wait_ms(5);
    controller->address = address;

    for (int i = 0; i < 3; i++) {
        controller->tdpool[i].next = 1;
    } SetUHCIRegisterW(*controller, USBSTS, 0x00FF);
}

void GetUHCIDeviceDescriptor(struct UHCIDevice* controller) {
    struct usb_setup_packet packet;
    packet.requesttype = 0x80;
    packet.request = 0x06;
    packet.value = 0x0100;
    packet.index = 0x00;
    packet.lenght = 0x012;

    controller->device_descriptor = (struct usb_device_descriptor*)ALIGN((uint64_t)kmalloc_align(18, 16), 16);
    memset(controller->device_descriptor, 0, sizeof(*controller->device_descriptor));

    memset(controller->tdpool, 0, sizeof(struct UHCITransferDescriptor) * 3);

    // TD 0
    controller->tdpool[0].buffer = (uint32_t)&packet;
    controller->tdpool[0].next = ((uint32_t)(&controller->tdpool[1]) & ~0xF) | (1 << 2);
    controller->tdpool[0].packet_header = (HEADER_TYPE_SETUP & 0xFF) | (controller->address << 8) | ((0x7 & 0x7ff) << 21);
    controller->tdpool[0].status = (1 << 23) | (controller->speed << 26) | (3 << 27);

    // TD 1
    controller->tdpool[1].buffer = (uint32_t)controller->device_descriptor;
    controller->tdpool[1].next = ((uint32_t)(&controller->tdpool[2]) & ~0xF) | (1 << 2);
    controller->tdpool[1].packet_header = (HEADER_TYPE_IN & 0xFF) | (controller->address << 8) | (1 << 19) | (((sizeof(struct usb_device_descriptor) - 1) & 0x7ff) << 21);
    controller->tdpool[1].status = (1 << 23) | (controller->speed << 26) | (3 << 27);

    // 2
    controller->tdpool[2].buffer = 0;
    controller->tdpool[2].next = 1;
    controller->tdpool[2].packet_header = (HEADER_TYPE_OUT & 0xFF) | (controller->address << 8) | (1 << 19) | (((0x7ff) & 0x7ff) << 21);
    controller->tdpool[2].status = (1 << 23) | (controller->speed << 26) | (3 << 27);

    controller->qhpool[0].vertical_pointer = GetQueueHeadEntry((uint32_t)(&controller->tdpool[0]), 0, FRAME_TYPE_TD);
    controller->qhpool[0].horizontal_pointer = GetQueueHeadEntry(0, 1, 0);
    SetFrameEntry(controller, 0, (uint32_t)(&controller->qhpool[0]), 0, FRAME_TYPE_QD);

    for (int i = 0; i < 3; i++) {
        #ifdef DEBUG
            printf("Waiting for packet %d (0x%p) to be readed...\n", i + 1, &controller->tdpool[i]);
        #endif
        while (controller->tdpool[i].status & (1 << 23)) PAUSE();

        // Check for errors
        if (controller->tdpool[i].status & ((1 << 22) | (1 << 21) | (1 << 20) | (1 << 18) | (1 << 17))) {
            #ifdef DEBUG
                DumpTd(controller->tdpool[i]);
            #endif
            printf("TD%d Failed!\n", i + 1);
            break;
        }
    }
    for (int i = 0; i < 3; i++)
        controller->tdpool[i].status = 0;
    SetUHCIRegisterW(*controller, USBSTS, 0x00FF);
}

struct usb_string_descriptor* GetUHCIString(struct UHCIDevice* controller, uint8_t index, uint16_t langid) {
    struct usb_setup_packet packet;
    packet.requesttype = 0x80;
    packet.request = 0x06;
    packet.value = (3 << 8) | index;
    packet.index = langid;
    packet.lenght = 1;

    struct usb_string_descriptor* descriptor = kmalloc(sizeof(struct usb_string_descriptor));
    GetUHCIDescriptor(controller, packet, descriptor, true);

    packet.lenght = descriptor->blength;
    GetUHCIDescriptor(controller, packet, descriptor, true);

    return descriptor;
}

void uhci_free(struct UHCIDevice* controller) {
    kfree(controller->framelist);
    kfree(controller->tdpool);
    kfree(controller->qhpool);
    kfree(controller);
}