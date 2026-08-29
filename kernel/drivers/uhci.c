//going to hate my life here we go

// damn like fr

#include "drivers/uhci.h"
#include "drivers/pci.h"
#include "drivers/pit.h"
#include "drivers/tables/irq.h"
#include "drivers/tables/isr.h"
#include "drivers/usb.h"
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

    
}

void uhci_interrupt(registers_t* r) {
    printf("UHCI Interrupt!\n");
}

struct UHCITransferDescriptor BakeTransferDescriptor(struct UHCITransferDescriptorRecipe recipe) {
    struct UHCITransferDescriptor td;

    // Set buffer
    td.buffer = recipe.BufferAddress;

    td.next = (recipe.PhysicalAddress.address & ~0xF) |
        (recipe.PhysicalAddress.type << 1) |
        (recipe.PhysicalAddress.depth << 2) |
        (recipe.PhysicalAddress.terminate << 0);

    // Set packet header
    td.packet_header = (recipe.PacketHeader.packet_type & 0xFF) |
        (recipe.PacketHeader.device << 8) |
        (recipe.PacketHeader.end_point << 15) |
        (recipe.PacketHeader.data_toggle << 19) |
        (((recipe.PacketHeader.max_lenght - 1) & 0x7ff) << 21);

    // Set status
    td.status = recipe.Status.actual_lenght |
        (recipe.Status.bse << 17) |
        (recipe.Status.timeoutcrc << 18) |
        (recipe.Status.non_ack << 19) |
        (recipe.Status.babble_detected << 20) |
        (recipe.Status.dbe << 21) |
        (recipe.Status.stalled << 22) |
        (recipe.Status.active << 23) |
        (recipe.Status.ioc << 24) |
        (recipe.Status.isochronous << 25) |
        (recipe.Status.low_speed << 26) |
        ((recipe.Status.err_counter & 0x3) << 27) |
        (recipe.Status.spd << 29);

    return td;
}

#define FRAME_TYPE_QD 1
#define FRAME_TYPE_TD 0

#define HEADER_TYPE_IN 0x69
#define HEADER_TYPE_OUT 0xE1
#define HEADER_TYPE_SETUP 0x2D

void SetFrameEntry(struct UHCIDevice* controller, uint32_t index, uint32_t entry, bool framedisable, bool type) {
    if (!controller->framelist) return;

    controller->framelist[index] = (entry & ~0xF) |
        (framedisable << 0) |
        (type << 1);
}

void LinkTDWith(struct UHCITransferDescriptor* td, bool type, bool terminate, bool depth, void* next) {
    td->next = ((uint32_t)next & ~0xF) |
        (type << 1) |
        (depth << 2) |
        (terminate << 0);
}

uint32_t GetQueueHeadEntry(uint32_t entry, bool framedisable, bool type) {
    return (entry & ~0xF) |
        (framedisable << 0) |
        (type << 1);
}

struct USBDevice uhci_init(struct PCIDevice device) {
    // uint16_t command_register = pci_readw(device.bus, device.slot, device.func, 0x4);
    // command_register &= ~0x3;
    // pci_writew(device.bus, device.slot, device.func, 0x4, command_register);
    // I dont know if this works, it should disable the io and memory decode, but with this or without it, it is still the same
    IOBar bar4 = PCIGetIOBar(device, 4);

    struct UHCIDevice* controller = kmalloc(sizeof(struct UHCIDevice));
    controller->header.device = device;
    controller->framelist = (FrameEntry*)ALIGN((uint64_t)kmalloc_align(1024 * sizeof(FrameEntry), 4096), 4096);
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
    pci_writew(device.bus, device.slot, device.func, 0xC0, 0x8F00);

    // Enable I/O Bus mastering
    pci_writew(device.bus, device.slot, device.func, 0x4, pci_readw(device.bus, device.slot, device.func, 0x4) | (1 << 2) | (1 << 0));

    // Disable interrupts
    SetUHCIRegisterW(*controller, USBINTR, 0);

    // Host controller reset
    SetUHCIRegisterW(*controller, USBCMD, ReadUHCIRegisterW(*controller, USBCMD) | CMD_HCRESET);
    pit_timer_wait_ms(25);

    int timeout = 1000;
    while ((ReadUHCIRegisterW(*controller, USBCMD) & CMD_HCRESET) && --timeout) pit_timer_wait_ms(1);
    if (!timeout) {
        set_printf_color(VGA_COLOR_LIGHT_RED);
            printf("Failed to restart UHCI Controller\n");
        set_printf_color(VGA_COLOR_WHITE);
    }

    // Set Frame List Base Address
    SetUHCIRegisterL(*controller, FRBASE, (uint32_t)(controller->framelist));
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

    struct USBDevice usb;
    usb.data = controller;
    usb.type = UHCICONTROLLER;
    usb.handler = UHCIHandler;
    usb.irq = pci_readb(device.bus, device.slot, device.func, 0x3c);

    // irq_install_handler(usb.irq, uhci_interrupt, 0);

    return usb;
err:
    kfree(controller->framelist);
    controller->framelist = NULL;
    kfree(controller);
    return (struct USBDevice){0};
}

void GetUHCIDescriptor(struct UHCIDevice* controller) {
    struct usb_setup_packet* setup_packet = kmalloc(sizeof(struct usb_setup_packet));
    setup_packet->requesttype = 0x06;
    setup_packet->request = 0x06;
    setup_packet->value = 0x20;
    setup_packet->index = 0x00;
    setup_packet->lenght = 0x009;

    // 0
    controller->tdpool[0].buffer = (uint32_t)setup_packet;

    controller->tdpool[0].next = ((uint32_t)(&controller->tdpool[1] - KERNEL_VIRT_BASE) & ~0xF) |
        (0 << 1) |
        (1 << 2) |
        (0 << 0);

    // Set packet header
    controller->tdpool[0].packet_header = (HEADER_TYPE_SETUP & 0xFF) |
        (0 << 8) |
        (0 << 15) |
        (0 << 19) |
        (((8 - 1) & 0x7ff) << 21);
    // printf("%x\n", controller->tdpool[0].packet_header);

    // Set status
    controller->tdpool[0].status = (uint32_t)0x7FF |
        (0 << 17) |
        (0 << 18) |
        (0 << 19) |
        (0 << 20) |
        (0 << 21) |
        (0 << 22) |
        (1 << 23) |
        (0 << 24) |
        (0 << 25) |
        (0 << 26) |
        (3 << 27) |
        (0 << 29);

    // 1
    char out[18];
    memset(out, 0, 18);
    controller->tdpool[1].buffer = (uint32_t)out;

    controller->tdpool[1].next = ((uint32_t)(&controller->tdpool[2]) & ~0xF) |
        (0 << 1) | // Type
        (1 << 2) | // Depth
        (0 << 0); // Terminate

    // Set packet header
    controller->tdpool[1].packet_header = (HEADER_TYPE_IN & 0xFF) | // Header type
        (0 << 8) | // Device
        (0 << 15) | // Endpoint
        (1 << 19) | // Toggle
        (((8 - 1) & 0x7ff) << 21); // Actual Length

    // Set status
    controller->tdpool[1].status = (uint32_t)0x7FF |
        (0 << 17) |
        (0 << 18) |
        (1 << 19) |
        (0 << 20) |
        (0 << 21) |
        (0 << 22) |
        (1 << 23) |
        (0 << 24) |
        (0 << 25) |
        (0 << 26) |
        (3 << 27) |
        (0 << 29);


    // 2
    controller->tdpool[2].buffer = (uint32_t)setup_packet;

    controller->tdpool[2].next = 1;

    // Set packet header
    controller->tdpool[2].packet_header = (HEADER_TYPE_OUT & 0xFF) | // Header type
        (0 << 8) | // Device
        (0 << 15) | // Endpoint
        (0 << 19) | // Toggle
        (((18 - 1) & 0x7ff) << 21); // Actual Length

    // Set status
    controller->tdpool[2].status = (uint32_t)0x7FF |
        (0 << 17) |
        (0 << 18) |
        (1 << 19) |
        (0 << 20) |
        (0 << 21) |
        (0 << 22) |
        (1 << 23) |
        (0 << 24) |
        (0 << 25) |
        (0 << 26) |
        (3 << 27) |
        (0 << 29);

    controller->qhpool->vertical_pointer = GetQueueHeadEntry((uint32_t)(&controller->tdpool[0]), 0, FRAME_TYPE_TD);
    controller->qhpool->horizontal_pointer = GetQueueHeadEntry(0, 1, 0);

    SetFrameEntry(controller, 0, (uint32_t)(&controller->qhpool[0] - KERNEL_VIRT_BASE), 0, FRAME_TYPE_QD);
    SetUHCIRegisterW(*controller, USBSTS, 0x00FF);

#if 0 // Please someone help me with this shit
    for (int i = 0; i < 3; i++) {
        printf("Waiting for packet %d (0x%p) to be readed...\n", i, &controller->tdpool[i]);
        pit_timer_wait_ms(10);
        while (controller->tdpool[i].status & (1 << 23)) PAUSE();

        // Check for errors
        if (controller->tdpool[i].status & ((1 << 22) | (1 << 18))) {
            // Stalled error
            uint8_t err_cause = (controller->tdpool[i].status >> 17) & 0x1F;
            printf("TD%d Failed! Status: 0x%x (Error code: 0x%x) %x\n", i, controller->tdpool[i].status, err_cause, out[0]);
        }
        // printf("Interrupt: %x\n", ReadUHCIRegisterW(*controller, USBSTS));
    }
#endif
}

void uhci_free(struct UHCIDevice* controller) {
    kfree(controller->framelist);
    kfree(controller);
}