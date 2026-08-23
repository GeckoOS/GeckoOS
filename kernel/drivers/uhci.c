//going to hate my life here we go

#include "drivers/uhci.h"
#include "drivers/apic/lapic.h"
#include "drivers/pci.h"
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

#define USBCMD  0x00 // USB Command
#define USBSTS  0x02 // USB Status
#define USBINTR 0x04 // USB Interrupt enable
#define FRNUM   0x06 // Frame number
#define FRBASE  0x08 // Frame List Base Address
#define SOFMOD  0x0C // Start Of Frame Modify
#define PORTSC1 0x10 // Port 1 Status/Control

// Command register bits
#define CMD_MAXP    (1 << 7) // Max Packet Size
#define CMD_CF      (1 << 6) // Configure Flag
#define CMD_SWDBG   (1 << 5) // Software Debug
#define CMD_FGR     (1 << 4) // Force Global Resume
#define CMD_EGSM    (1 << 3) // Enter Global Suspend Mode
#define CMD_GRESET  (1 << 2) // Global Reset
#define CMD_HCRESET (1 << 1) // Host Controller Reset
#define CMD_RS      (1 << 0) // Run/Stop

// Status register bits
#define STS_HCHALT  (1 << 5)
#define STS_HCPR    (1 << 4) // Host Controller Process Error
#define STS_HSE     (1 << 3) // Host System Error
#define STS_RD      (1 << 2) // Resume Detecte
#define STS_UEI     (1 << 1) // USB Error Interrupt
#define STS_UI      (1 << 0) // USB Interrupt

// Interrupt enable registers bits
#define INTR_SPIE   (1 << 3) // Short Packet Interrupt Enable
#define INTR_IOCE   (1 << 2) // Interrupt on Complete (IOC) Enable
#define INTR_RIE    (1 << 1) // Resume Interrupt Enable
#define INTR_TIE    (1 << 0) // Timeout/CRC Interrupt Enable

// Port registers bits
#define PRT_SUSPEND (1 << 12) // Suspend (R/W)
#define PRT_RESET   (1 << 9)  // Port reset (R/W)
#define PRT_LSDA    (1 << 8)  // Low Speed Device Attached (R/O)
#define PRT_RD      (1 << 6)  // Resume Detect (R/W)
#define PRT_LS      (1 << 4)  // Line status (R/O) (2 Bits)
#define PRT_PEDC    (1 << 3)  // Port Enable/Disable Change (R/WC)
#define PRT_PED     (1 << 2)  // Port Enabled/Disabled (R/W)
#define PRT_CSC     (1 << 1)  // Connect Status Change (R/WC)
#define PRT_CCS     (1 << 0)  // Current Connect Status (R/O)

#define PRT_RWC     (PRT_CSC | PRT_PEDC)

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

void UHCIActivatePorts(struct UHCIDevice controller) { // I dont know what to do with these port
    for (int i = 0; i <= 4; i++) {
        uint8_t reg = PORTSC1 + (i * 2);

        uint16_t port = ReadUHCIRegisterW(controller, reg);
        if (port == 0xffff || !(port & (1 << 7))) continue;

        #ifdef DEBUG
            printf("  The %d UHCI Controller Port is valid\n", i + 1);
        #endif

        int tick_start = lapic_timer_tick;

        SetUHCIRegisterW(controller, reg, port | PRT_RESET);
        while (lapic_timer_tick - (tick_start < 200)) HALT();

        tick_start = lapic_timer_tick;
        SetUHCIRegisterW(controller, reg, port & ~PRT_RESET);
        while (lapic_timer_tick - (tick_start < 250)) HALT();

        // SetUHCIRegisterW(controller, reg, port | PRT_);
    }
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
        (recipe.PacketHeader.end_point << 15) | // TODO: UPDATE THIS
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

    struct UHCIDevice* controller = kmalloc_4m(sizeof(struct UHCIDevice));
    controller->header.device = device;
    controller->framelist = kmalloc(1024 * sizeof(FrameEntry));
    memset(controller->framelist, 0, 1024 * sizeof(FrameEntry));

    if (!bar4) {
        set_printf_color(VGA_COLOR_LIGHT_RED);
            printf("  UHCI Controller is invalid!\n", device.bus, device.slot, device.func);
        set_printf_color(VGA_COLOR_WHITE);
        goto err;
    }

    uint16_t ioport = bar4 & ~0x3;
    controller->header.ioport = ioport;

    #ifdef DEBUG
        printf("  [DEBUG]: UHCI Controller with IO Port = 0x%x and header type = 0x%x\n", ioport, pci_readb(device.bus, device.slot, device.func, 0xc + 0x3));
    #endif

    // Take control from BIOS
    SetUHCIRegisterL(*controller, 0xC0, 0x8F00);

    // Enable I/O Bus mastering
    pci_writew(device.bus, device.slot, device.func, 0x4, pci_readw(device.bus, device.slot, device.func, 0x4) | (1 << 2));

    // Disable interrupts
    SetUHCIRegisterW(*controller, USBINTR, 0);

    // Host controller reset
    SetUHCIRegisterW(*controller, USBCMD, ReadUHCIRegisterW(*controller, USBCMD) | CMD_HCRESET);
    while (ReadUHCIRegisterW(*controller, 0) & CMD_HCRESET);

    // Set Frame List Base Address
    SetUHCIRegisterL(*controller, FRBASE, (uint32_t)(controller->framelist));
    printf("%x %p\n", ReadUHCIRegisterL(*controller, FRBASE), controller->framelist);
    SetUHCIRegisterW(*controller, FRNUM, 0x0);
    SetUHCIRegisterW(*controller, SOFMOD, 0x40);

    // Clean the status register
    // SetUHCIRegisterW(*controller, USBSTS, 0xFFFF);

    // Enable it
    SetUHCIRegisterW(*controller, USBCMD, CMD_RS | CMD_MAXP);

    struct USBDevice usb;
    usb.data = (struct BasicUSBHeader*)controller;
    usb.type = UHCICONTROLLER;
    usb.handler = UHCIHandler;
    usb.irq = pci_readb(device.bus, device.slot, device.func, 0x3c);

    // irq_install_handler(usb.irq, uhci_interrupt, 0);

    // controller->ports = UHCIFindPorts(*controller);
    // We are gonna use the 2 ports that there are for now

    return usb;
err:
    kfree(controller->framelist);
    kfree(controller->qhpool);
    kfree(controller->tdpool);
    controller->framelist = NULL;
    kfree(controller);
    return (struct USBDevice){0};
}

void GetUHCIDescriptor(struct UHCIDevice* controller) {
    struct usb_setup_packet {
        uint8_t requesttype;
        uint8_t request;
        uint16_t value;
        uint16_t index;
        uint16_t lenght;
    } setup_packet;

    setup_packet.requesttype = 0x80;
    setup_packet.request = 0x06;
    setup_packet.value = 0x20;
    setup_packet.index = 0x00;
    setup_packet.lenght = 0x009;

    uint64_t tick_start = lapic_timer_tick;
    SetUHCIRegisterW(*controller, 0x10, ReadUHCIRegisterW(*controller, 0x10) | PRT_RESET);
    while (lapic_timer_tick - tick_start < 10) {
        HALT();
    }
    SetUHCIRegisterW(*controller, 0x10, ReadUHCIRegisterW(*controller, 0x10) & ~PRT_RESET);
    SetUHCIRegisterW(*controller, 0x10, ReadUHCIRegisterW(*controller, 0x10) | PRT_PED);

    char out[18];
    memset(out, 0, 18);

    struct UHCITransferDescriptorRecipe td_recipe;
    memset(&td_recipe, 0, sizeof(td_recipe));

    td_recipe.Status.active = true;
    td_recipe.Status.low_speed = true;

    // printf("%x\n", &setup_packet - KERNEL_VIRT_BASE);

    td_recipe.BufferAddress = (uint32_t)(&setup_packet);
    td_recipe.PhysicalAddress.depth = true;
    td_recipe.PhysicalAddress.type = FRAME_TYPE_TD;
    td_recipe.PacketHeader.packet_type = HEADER_TYPE_SETUP;
    td_recipe.PacketHeader.max_lenght = 8;
    td_recipe.PacketHeader.data_toggle = false;
    td_recipe.PhysicalAddress.address = (uint32_t)(&controller->tdpool[1]);

    controller->tdpool[0] = BakeTransferDescriptor(td_recipe);

    td_recipe.BufferAddress = (uint32_t)(out);
    td_recipe.PacketHeader.packet_type = HEADER_TYPE_IN;
    td_recipe.PacketHeader.max_lenght = 18;
    td_recipe.PacketHeader.data_toggle = true;
    td_recipe.PhysicalAddress.address = (uint32_t)(&controller->tdpool[2]);

    controller->tdpool[1] = BakeTransferDescriptor(td_recipe);

    td_recipe.BufferAddress = 0;
    td_recipe.PacketHeader.packet_type = HEADER_TYPE_OUT;
    td_recipe.PacketHeader.max_lenght = 0x7FF;
    td_recipe.PhysicalAddress.terminate = true;
    td_recipe.PhysicalAddress.depth = false;
    td_recipe.PhysicalAddress.address = 0;

    controller->tdpool[2] = BakeTransferDescriptor(td_recipe);

    controller->qhpool->vertical_pointer = GetQueueHeadEntry((uint32_t)(&controller->tdpool[0]), 0, FRAME_TYPE_TD);
    controller->qhpool->horizontal_pointer = GetQueueHeadEntry(0, 1, 0);

    // SetFrameEntry(controller, 0, (uint32_t)(controller->qhpool), 0, FRAME_TYPE_QD);

    while (out[0] == 0) {
        SetUHCIRegisterL(*controller, FRBASE, (uint32_t)(controller->framelist));
        printf("%x %x %x %x\n", ReadUHCIRegisterW(*controller, FRNUM & 0x3FF), (controller->tdpool[0].status & (1 << 23)), ReadUHCIRegisterL(*controller, FRBASE), (uint32_t)controller->framelist);
        uint16_t cmd = ReadUHCIRegisterW(*controller, USBCMD);
        uint16_t sts = ReadUHCIRegisterW(*controller, USBSTS);

        // printf("UHCI CMD = %04x\n", cmd);
        // printf("UHCI STS = %04x %d\n", sts, ((uintptr_t)controller->framelist & 0xFFF) == 0);
        // printf("%p %p\n", (controller->framelist), (uint64_t)(controller->framelist));
    }
    printf("It neegy\n");
}

void uhci_free(struct UHCIDevice* controller) {
    kfree(controller->framelist);
    kfree(controller->tdpool);
    kfree(controller->qhpool);
    kfree(controller);
}