#pragma once
//driver for usb devices

#include "drivers/pci.h"
#include "drivers/usb.h"
#include <stdbool.h>
#include <stdint.h>

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
    struct BasicUSBHeader header;
    FrameEntry* framelist;
    struct UHCIQueueHead qhpool[6];
    struct UHCITransferDescriptor tdpool[24];
};

// Recipes
struct UHCITransferDescriptorRecipe {
    struct {
        uint32_t address;
        bool depth;
        bool type;
        bool terminate;
    } PhysicalAddress;
    struct {
        bool spd;
        uint8_t err_counter;
        bool low_speed;
        bool isochronous;
        bool ioc;
        bool active;
        bool stalled;
        bool dbe;
        bool babble_detected;
        bool non_ack;
        bool timeoutcrc;
        bool bse;
        uint16_t actual_lenght;
    } Status;
    struct {
        uint16_t max_lenght;
        bool data_toggle;
        uint8_t end_point;
        uint8_t device;
        uint32_t packet_type;
    } PacketHeader;
    uint32_t BufferAddress;
};

struct USBDevice uhci_init(struct PCIDevice device);

uint16_t ReadUHCIRegisterW(struct UHCIDevice controller, uint8_t reg);
uint32_t ReadUHCIRegisterL(struct UHCIDevice controller, uint8_t reg);
void GetUHCIDescriptor(struct UHCIDevice* controller);