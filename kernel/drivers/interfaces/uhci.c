//going to hate my life here we go

// damn like fr

#include <drivers/interfaces/uhci.h>
#include "drivers/hid/keyboard.h"
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

#define UHCI_MAX_CHILDRENS 7

#define ReadUHCIRegisterW(controller, reg) inw(controller.ioport + reg)
#define ReadUHCIRegisterL(controller, reg) inl(controller.ioport + reg)
#define SetUHCIRegisterW(controller, reg, data) outw(controller.ioport + reg, data)
#define SetUHCIRegisterL(controller, reg, data) outl(controller.ioport + reg, data)

static bool SendUHCIPacket(struct BasicUSBHeader* header, struct usb_setup_packet setup_packet, void* buffer, bool twice);
static void SetUHCIInterruptTranfers(struct BasicUSBHeader* uhci);
static void SetUHCIBulkTransferAt(struct BasicUSBHeader* uhci, uint8_t interval, void* buffer, uint16_t size);
static bool IsUHCIConnected(struct BasicUSBHeader* uhci);

// Helpers
static uint32_t GetQueueHeadEntry(uint32_t entry, bool framedisable, bool type) {
    return (entry & ~0xF) |
        (framedisable << 0) |
        (type << 1);
}
static void SetFrameEntry(struct UHCIDevice* controller, uint32_t index, uint32_t entry, bool framedisable, bool type) {
    if (!controller->framelist) return;

    controller->framelist[index] = (entry & ~0xF) |
        (framedisable << 0) |
        (type << 1);
}
static void SetTDOnIndex(struct UHCIDevice* uhci, int index, struct UHCITransferDescriptor td) {
    if (!uhci) return;
    uhci->tdpool[index] = td;
}
static void SetQHOnIndex(struct UHCIDevice* uhci, int index, struct UHCIQueueHead qh) {
    if (!uhci) return;
    uhci->qhpool[index] = qh;
}

static int uhci_count_ports(struct UHCIDevice uhci) {
    int port;
    for (port = 0; port < (uhci.ioport - PORTSC1) / 2; port++) {
        unsigned portsts = ReadUHCIRegisterW(uhci, PORTSC1 + (port * 2));
        if (!(portsts & 0x0080) || portsts == 0xFFFF) break;
    }
    if (port > UHCI_MAX_CHILDRENS) {
        printf("UHCI Controller has more ports than normal");
        return UHCI_MAX_CHILDRENS;
    }
    return port;
}
static bool uhci_reset_port(struct UHCIDevice* controller, int i) {
    uint8_t reg = PORTSC1 + (i * 2);

    uint16_t port = ReadUHCIRegisterW((*controller), reg);
    if (port & 1 && port != 0xFFFF) {
        // Enable reset
        uint16_t cmd = port & ~0xA;
        SetUHCIRegisterW((*controller), reg, cmd);
        pit_timer_wait_ms(50);

        // Disable reset
        cmd &= ~0x200;
        SetUHCIRegisterW((*controller), reg, cmd);
        pit_timer_wait_ms(10);

        SetUHCIRegisterW((*controller), reg, (ReadUHCIRegisterW((*controller), reg) & ~0x13) | 0xA);

        // Enable port
        SetUHCIRegisterW((*controller), reg, 0xE);

        int timeout = 100;
        while (!(ReadUHCIRegisterW((*controller), reg) & (1 << 2)) && --timeout) pit_timer_wait_ms(1);
        if (!timeout) {
            set_printf_color(VGA_COLOR_LIGHT_RED);
                printf("Failed to enable UHCI Controller's port %d\n", i + 1);
            set_printf_color(VGA_COLOR_WHITE);
            return false;
        }
    }
    return true;
}

void uhci_interrupt(registers_t* _) {
    for (int i = 0; i < USBDevices_Count; i++) {
        if (USBDevices[i].type != UHCICONTROLLER) continue;

        struct UHCIDevice* controller = (struct UHCIDevice*)USBDevices[i].data.controller;

        uint16_t status = ReadUHCIRegisterW((*controller), 0x2);
        SetUHCIRegisterW((*controller), 0x2, status | 1);

        if ((status & 1)) {
            if (!USBDevices[i].IsConnected(&USBDevices[i].data)) {
                printf("UHCI Usb was disconnected %x\n", ReadUHCIRegisterW((*controller), 0x2));
                controller->tdpool[0].packet_header &= ~0x80000;
                continue;
            }
            if ((controller->tdpool[0].status & ~0x800000)) {
                controller->qhpool[INTERRUPT_TRANSFER_INTERVAL_8MS].vertical_pointer = 1;

                // Now we parse what the controller told us
                ManageKeyboardReport(USBDevices[i]);

                controller->tdpool[0].status = 0x1800000;
                controller->tdpool[0].packet_header ^= 0x80000;

                controller->qhpool[INTERRUPT_TRANSFER_INTERVAL_8MS].vertical_pointer = GetQueueHeadEntry((uint32_t)&controller->tdpool[0], 0, FRAME_TYPE_TD);
            }
        }
    }
}

struct USBDevice* uhci_init(struct PCIDevice device) {
    // return NULL;

    IOBar bar4 = PCIGetIOBar(device, 4);

    struct UHCIDevice* controller = kmalloc(sizeof(struct UHCIDevice));
    controller->framelist = (FrameEntry*)ALIGN((uint64_t)kmalloc_align(1024 * sizeof(FrameEntry), 4096), 4096);
    controller->tdpool = (struct UHCITransferDescriptor*)ALIGN((uint64_t)kmalloc_align(32 * sizeof(struct UHCITransferDescriptor), 16), 16);
    controller->qhpool = (struct UHCIQueueHead*)ALIGN((uint64_t)kmalloc_align(16 * sizeof(struct UHCIQueueHead), 16), 16);
    controller->device = device;

    memset(controller->framelist, 0, 1024 * sizeof(FrameEntry));

    if (!bar4) {
        printc("  UHCI Controller is invalid!\n", VGA_COLOR_LIGHT_RED);
        goto err;
    }

    controller->ioport = bar4 & ~0x3;

    // Take control from BIOS
    // Enable I/O Bus mastering
    pci_writew(device.bus, device.slot, device.func, 0xC0, 0x2000);
    pci_writew(device.bus, device.slot, device.func, 0x4, pci_readw(device.bus, device.slot, device.func, 0x4) | 0x5);

    // Host controller reset
    SetUHCIRegisterW((*controller), USBCMD, ReadUHCIRegisterW((*controller), USBCMD) | CMD_HCRESET);
    pit_timer_wait_ms(25);

    int timeout = 1000;
    while ((ReadUHCIRegisterW((*controller), USBCMD) & CMD_HCRESET) && --timeout) pit_timer_wait_ms(1);
    if (!timeout) {
        printc("Failed to restart UHCI Controller\n", VGA_COLOR_LIGHT_RED);
        goto err;
    }

    // Set Frame List Base Address
    SetUHCIRegisterL((*controller), FRBASE, (uint32_t)(controller->framelist) & ~0xFFF);
    SetUHCIRegisterW((*controller), FRNUM, 0x0);
    SetUHCIRegisterW((*controller), SOFMOD, 0x40);

    // Clean the status register
    SetUHCIRegisterW((*controller), USBSTS, 0xFFFF);

    // Enable it
    SetUHCIRegisterW((*controller), USBCMD, CMD_RS | CMD_MAXP);
    pit_timer_wait_ms(5);

    if (ReadUHCIRegisterW((*controller), USBSTS) & STS_HCHALT) {
        printf("UHCI error when enabling it!");
        goto err;
    }

    // Enable interrupts (iOC)
    SetUHCIRegisterW((*controller), USBINTR, INTR_IOCE);

    irq_install_handler(pci_readb(device.bus, device.slot, device.func, 0x3C), uhci_interrupt, 0);
    // printf("%d\n", pci_readb(device.bus, device.slot, device.func, 0x3C)); // Is it normal that this irq is the same as the e1000 irq (Yes it is)

    struct USBDevice* devices = kmalloc(sizeof(struct USBDevice*) * 2);
    if (!devices) return NULL;
    memset(devices, 0, sizeof(struct USBDevice) * 2);

    // Look if a usb is connected in any of the n ports
    int max = uhci_count_ports(*controller);
    for (int i = 0; i < max; i++) {
        uint16_t sts = ReadUHCIRegisterW((*controller), PORTSC1 + (i * 2));
        if (!uhci_reset_port(controller, i) || !(sts & 1)) {
            devices[i].data.controller = NULL;
            continue;
        }

        devices[i].data.controller = (struct PCIDevice*)controller;
        devices[i].data.controller_reserved = kmalloc(sizeof(uint8_t) * 2);
        /*
            An array of two uint8_t, one for the device address of that device (for the uhci functions), and the second one is for the speed (is lowspeed?)
        */
        (((uint8_t*)devices[i].data.controller_reserved)[1]) = sts & (1 << 8);
        (*(uint8_t*)devices[i].data.controller_reserved) = 0;
        devices[i].type = UHCICONTROLLER;

        devices[i].SendPacket = SendUHCIPacket;
        devices[i].InitInterruptTranfers = SetUHCIInterruptTranfers;
        devices[i].SetInterruptTransfer = SetUHCIBulkTransferAt;
        devices[i].IsConnected = IsUHCIConnected;

        memset(&devices[i].data.device_descriptor, 0, sizeof(devices[i].data.device_descriptor));
        if (!devices[i].SendPacket(&devices[i].data, (struct usb_setup_packet){
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
        if (!devices[i].SendPacket(&devices[i].data, (struct usb_setup_packet){
                .requesttype = 0x00,
                .request = REQUEST_SET_ADDRESS,
                .value = i + 1,
                .index = 0x00,
                .lenght = 0x00
            }, 0, true)) {
                printf("UHCI Controller port %d: Failed to set address\n", i);
                devices[i].data.controller = NULL;
                continue;
            }

        (*(uint8_t*)devices[i].data.controller_reserved) = i + 1;
    }

    return devices;
err:
    kfree(controller->framelist);
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

static bool SetUHCIControlTransfer(struct BasicUSBHeader* uhci, struct usb_setup_packet setup, void* buffer, bool wait /* Waits for all of the packets to be readed */, bool no_response) {
    struct UHCIDevice* controller = (struct UHCIDevice*)uhci->controller;
    int packets = 1;

    bool speed = ((uint8_t*)uhci->controller_reserved)[1];
    SetTDOnIndex(controller, 0, (struct UHCITransferDescriptor){
        .buffer = (uint32_t)&setup,
        .next = ((uint32_t)(&controller->tdpool[1]) & ~0xF) | (1 << 2),
        .packet_header = (HEADER_TYPE_SETUP & 0xFF) | ((*(uint8_t*)uhci->controller_reserved) << 8) | ((0x7 & 0x7ff) << 21),
        .status = (1 << 23) | (speed << 26) | (3 << 27)
    }); // Setup packet

    // Put more data packets if the setup packet wants more than the max limit
    if (((setup.lenght > uhci->device_descriptor.bMaxpacketsize) && uhci->device_descriptor.bMaxpacketsize) && !no_response) {
        int max = (int)(setup.lenght / uhci->device_descriptor.bMaxpacketsize) + 1;
        int n = 0;

        for (int i = 1; i <= max; i++) {
            uint32_t size = (i == max ? (setup.lenght - (uhci->device_descriptor.bMaxpacketsize * (i - 1))) : uhci->device_descriptor.bMaxpacketsize);

            SetTDOnIndex(controller, packets, (struct UHCITransferDescriptor){
                .buffer = (uint32_t)buffer + n,
                .next = ((uint32_t)(&controller->tdpool[packets + 1]) & ~0xF) | (1 << 2),
                .packet_header = (HEADER_TYPE_IN & 0xFF) | ((*(uint8_t*)uhci->controller_reserved) << 8) | ((bool)(i % 2) << 19) | (((size - 1) & 0x7ff) << 21),
                .status = (1 << 23) | (speed << 26) | (3 << 27) | (1 << 29)
            }); // Data packet
            n += size;
            packets++;
        }
    } else {
        SetTDOnIndex(controller, 1, (struct UHCITransferDescriptor){
            .buffer = (uint32_t)buffer,
            .next = no_response ? 1 : ((uint32_t)(&controller->tdpool[packets + 1]) & ~0xF) | (1 << 2),
            .packet_header = (HEADER_TYPE_IN & 0xFF) | ((*(uint8_t*)uhci->controller_reserved) << 8) | (1 << 19) | (((setup.lenght - 1) & 0x7ff) << 21),
            .status = (1 << 23) | (speed << 26) | (3 << 27) | (1 << 29)
        }); // Data packet
        packets++;
    }

    if (!no_response) { // For requests like "set_address" that are just two packets (SETUP and STATUS)
        SetTDOnIndex(controller, packets, (struct UHCITransferDescriptor){
            .buffer = 0,
            .next = 1,
            .packet_header = (HEADER_TYPE_OUT & 0xFF) | ((*(uint8_t*)uhci->controller_reserved) << 8) | (1 << 19) | ((0 & 0x7ff) << 21),
            .status = (1 << 23) | (speed << 26) | (3 << 27)
        }); // Status packet
        packets++;
    }

    SetQHOnIndex(controller, 0, (struct UHCIQueueHead){
        .vertical_pointer = GetQueueHeadEntry((uint32_t)&controller->tdpool[0], 0, FRAME_TYPE_TD),
        .horizontal_pointer = 1
    });
    SetFrameEntry(controller, 0, (uint32_t)&controller->qhpool[0], 0, FRAME_TYPE_QH);

    if (wait)
        for (int i = 0; i < packets; i++) {
            #ifdef DEBUG
                printf(".");
            #endif
            while (controller->tdpool[i].status & (1 << 23)) PAUSE(); // Wait for the TD active bit to turn off

            // Check for errors
            if (controller->tdpool[i].status & 0x7E0000) {
                #ifdef DEBUG
                    DumpTd(controller->tdpool[i]);
                    printf("TD%d Failed!\n", i + 1);
                #endif
                return false;
                break;
            }
    } SetUHCIRegisterW((*controller), USBSTS, 0x00FF);

    return true;
}

static void SetUHCIInterruptTranfers(struct BasicUSBHeader* uhci) {
    struct UHCIDevice* controller = (struct UHCIDevice*)uhci->controller;

    for (int i = 5; i > 0; i--) {
        controller->qhpool[i * 2].horizontal_pointer = GetQueueHeadEntry((uint32_t)&controller->qhpool[(i * 2) - 2], 0, FRAME_TYPE_QH);
        controller->qhpool[i * 2].vertical_pointer = 1;   // T=1 → (grok redesigned this part of code) empty
    }
    controller->qhpool[0].horizontal_pointer = 1;
    controller->qhpool[0].vertical_pointer   = 1;

    for (int i = 0; i < 1024; i++) {
        if ((i & 31) == 0) // Every 32ms
            SetFrameEntry(controller, i, (uint32_t)&controller->qhpool[INTERRUPT_TRANSFER_INTERVAL_32MS], 0, FRAME_TYPE_QH);
        else if ((i & 15) == 0) // Every 16ms
            SetFrameEntry(controller, i, (uint32_t)&controller->qhpool[INTERRUPT_TRANSFER_INTERVAL_16MS], 0, FRAME_TYPE_QH);
        else if ((i & 7) == 0) // Every 8ms
            SetFrameEntry(controller, i, (uint32_t)&controller->qhpool[INTERRUPT_TRANSFER_INTERVAL_8MS], 0, FRAME_TYPE_QH);
        else if ((i & 3) == 0) // Every 4ms
            SetFrameEntry(controller, i, (uint32_t)&controller->qhpool[INTERRUPT_TRANSFER_INTERVAL_4MS], 0, FRAME_TYPE_QH);
        else if ((i & 1) == 0) // Every 2ms
            SetFrameEntry(controller, i, (uint32_t)&controller->qhpool[INTERRUPT_TRANSFER_INTERVAL_2MS], 0, FRAME_TYPE_QH);
        else // Every 1ms
            SetFrameEntry(controller, i, (uint32_t)&controller->qhpool[INTERRUPT_TRANSFER_INTERVAL_1MS], 0, FRAME_TYPE_QH);
    }
}
static void SetUHCIBulkTransferAt(struct BasicUSBHeader* uhci, uint8_t interval, void* buffer, uint16_t size) {
    struct UHCIDevice* controller = (struct UHCIDevice*)uhci->controller;
    // if (!uhci->controller_reserved || !uhci->controller) return;

    bool speed = ((uint8_t*)uhci->controller_reserved)[1];

    int endpoint_in;
    for (endpoint_in = 0; endpoint_in < uhci->endpoints_count; endpoint_in++)
        if (uhci->endpoint[endpoint_in].bEndpointAddress & (1 << 7)) break;
    // if (!endpoint_in) return;

    SetTDOnIndex(controller, 0, (struct UHCITransferDescriptor){
        .buffer = (uint32_t)buffer,
        .next = 1,
        .packet_header = (HEADER_TYPE_IN & 0xFF) | ((*(uint8_t*)uhci->controller_reserved) << 8) | (1 << 19) | ((uhci->endpoint[endpoint_in].bEndpointAddress & 0xF) << 15) | (((size - 1) & 0x7ff) << 21),
        .status = (1 << 23) | (speed << 26) | (3 << 27) | (1 << 24)
    });

    controller->qhpool[interval].vertical_pointer = GetQueueHeadEntry((uint32_t)&controller->tdpool[0], 0, FRAME_TYPE_TD);
}

static bool SendUHCIPacket(struct BasicUSBHeader* header, struct usb_setup_packet setup_packet, void* buffer, bool twice) { return SetUHCIControlTransfer(header, setup_packet, buffer, true, twice); }
static bool IsUHCIConnected(struct BasicUSBHeader* uhci) {
    if (!uhci->controller) return false;
    return ReadUHCIRegisterW((*(struct UHCIDevice*)uhci->controller), 0x10 + (2 * ((*(uint8_t*)uhci->controller_reserved) - 1))) & 1;
}

#if 0

void uhci_free(struct UHCIDevice* controller) {
    kfree(controller->framelist);
    kfree(controller->tdpool);
    kfree(controller->qhpool);
    kfree(controller);
}

#endif