//going to hate my life here we go

#include "drivers/uhci.h"
#include "drivers/pci.h"
#include "drivers/tables/irq.h"
#include "drivers/tables/isr.h"
#include "drivers/usb.h"
#include "mem.h"
#include "ports.h"
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

int UHCIFindPorts(struct UHCIDevice controller) { // I dont know what to do with these port
    for (int i = 0; i <= 4; i++) {
        uint8_t reg = PORTSC1 + (i * 2);

        uint16_t port = ReadUHCIRegisterW(controller, reg);
        if (port == 0xffff || !(port & (1 << 7))) continue;

        #ifdef DEBUG
            printf("  The %d UHCI Controller Port is valid\n", i + 1);
        #endif
    }
}

void UHCIHandler(struct BasicUSBHeader* header) {
    struct UHCIDevice* controller = (struct UHCIDevice*)header;

    #ifdef DEBUG
        printf("UHCI Handler function: %s\n", __func__);
    #endif
}

void uhci_interrupt(registers_t* r) {
    printf("UHCI Interrupt!\n");
}

struct USBDevice uhci_init(struct PCIDevice device) {
    // uint16_t command_register = pci_readw(device.bus, device.slot, device.func, 0x4);
    // command_register &= ~0x3;
    // pci_writew(device.bus, device.slot, device.func, 0x4, command_register);
    // I dont know if this works, it should disable the io and memory decode, but with this or without it, it is still the same
    IOBar bar4 = PCIGetIOBar(device, 4);

    struct UHCIDevice* controller = kmalloc(sizeof(struct UHCIDevice));
    controller->header.device = device;
    controller->framelist = kmalloc(1024 * sizeof(FrameEntry));
    memset(controller->framelist, 0, 1024 * sizeof(FrameEntry));
    controller->qhpool = kmalloc(sizeof(struct UHCIQueueHead) * 8);
    controller->tdpool = kmalloc(sizeof(struct UHCITransferDescriptor) * 32);

    if (!bar4) {
        set_printf_color(VGA_COLOR_LIGHT_RED);
            printf("  UHCI Controller is invalid!\n", device.bus, device.slot, device.func);
        set_printf_color(VGA_COLOR_WHITE);
        goto err;
    }

    uint16_t ioport = bar4 & ~0x3;
    controller->header.ioport = ioport;

    #ifdef DEBUG
        set_printf_color(VGA_COLOR_MAGENTA);
            printf("  [DEBUG]: UHCI Controller with IO Port = 0x%x and header type = 0x%x\n", ioport, pci_readb(device.bus, device.slot, device.func, 0xc + 0x3));
        set_printf_color(VGA_COLOR_WHITE);
    #endif

    // TODO: When a decent timer gets added, update this by replacing the obsolete fors that act as timers to the decent timer

    // Take control from BIOS
    SetUHCIRegisterL(*controller, 0xC0, 0x8f00);

    // Disable interrupts
    SetUHCIRegisterW(*controller, USBINTR, 0);

    // Host controller reset
    SetUHCIRegisterW(*controller, USBCMD, ReadUHCIRegisterW(*controller, USBCMD) | CMD_HCRESET);
    while ((ReadUHCIRegisterW(*controller, 0) & CMD_HCRESET) != 0) HALT();

    // Set Frame List Base Address
    SetUHCIRegisterL(*controller, FRBASE, (uint32_t)(size_t)controller->framelist);
    SetUHCIRegisterW(*controller, FRNUM, 0x0);
    SetUHCIRegisterW(*controller, SOFMOD, 0x40);

    // Clean the status register
    SetUHCIRegisterW(*controller, USBSTS, 0xFFFF);

    // Enable it
    SetUHCIRegisterW(*controller, USBCMD, CMD_RS);

    // controller.ports = UHCIFindPorts(controller);
    
    struct USBDevice usb;
    usb.data = (struct BasicUSBHeader*)controller;
    usb.type = UHCICONTROLLER;
    usb.handler = UHCIHandler;
    usb.irq = pci_readb(device.bus, device.slot, device.func, 0x3c);

    // irq_install_handler(usb.irq, uhci_interrupt, 0);

    return usb;
err:
    kfree(controller->framelist);
    kfree(controller->qhpool);
    kfree(controller->tdpool);
    controller->framelist = NULL;
    kfree(controller);
    return (struct USBDevice){0};
}