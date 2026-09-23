#pragma once
//driver for usb devices

#include "drivers/pci.h"
#include "drivers/usb.h"
#include <stdbool.h>
#include <stdint.h>

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

#define FRAME_TYPE_QH 1
#define FRAME_TYPE_TD 0

#define HEADER_TYPE_IN 0x69
#define HEADER_TYPE_OUT 0xE1
#define HEADER_TYPE_SETUP 0x2D

typedef uint32_t FrameEntry;

struct UHCIQueueHead {
    uint32_t horizontal_pointer;
    uint32_t vertical_pointer;
} __attribute__((packed));

struct UHCITransferDescriptor {
    volatile uint32_t next;
    volatile uint32_t status;
    volatile uint32_t packet_header;
    volatile uint32_t buffer;
    uint8_t pad[16];
} __attribute__((packed));

struct UHCIDevice {
    struct PCIDevice device;
    FrameEntry* framelist;
    struct UHCIQueueHead* qhpool;
    struct UHCITransferDescriptor* tdpool;
    uint16_t ioport;
    uint16_t max_packet_size;
    uint8_t device_address;
};

struct USBDevice* uhci_init(struct PCIDevice device);

// bool GetUHCIDescriptor(struct UHCIDevice* controller, struct usb_setup_packet setup, void* buffer, bool wait, bool twice);