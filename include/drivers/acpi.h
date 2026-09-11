#pragma once

#include "drivers/tables/isr.h"
#include <stdint.h>

struct acpi_rsdp_v1 {
    char signature[8];
    uint8_t checksum;
    char oem_id[6];
    uint8_t revision;
    uint32_t rsdt_addr;
} __attribute__((packed));

struct acpi_rsdp_v2 {
    struct acpi_rsdp_v1 v1;
    uint32_t length;
    uint64_t xsdt_addr;
    uint8_t extended_checksum;
    uint8_t reserved[3];
} __attribute__((packed));

typedef struct acpi_header {
    char signature[4];
    uint32_t length;
    uint8_t revision;
    uint8_t checksum;
    char oem_id[6];
    char oem_table_id[8];
    uint32_t oem_revision;
    uint32_t creator_id;
    uint32_t creator_revision;
}__attribute__((packed)) acpi_header_t;

typedef struct acpi_madt {
    struct acpi_header header;
    uint32_t lapic_addr;
    uint32_t flags;
    // entries follow
} __attribute__((packed)) acpit_madt_t;

struct madt_entry_header {
    uint8_t type;
    uint8_t length;
} __attribute__((packed));

struct madt_ioapic {
    uint8_t type;      // 1
    uint8_t length;    // 12
    uint8_t ioapic_id;
    uint8_t reserved;
    uint32_t addr;
    uint32_t gsi_base;
} __attribute__((packed));

struct madt_iso {
    uint8_t type;      // 2
    uint8_t length;    // 10
    uint8_t bus;
    uint8_t irq;
    uint32_t gsi;
    uint16_t flags;
} __attribute__((packed));

typedef struct {
    acpi_header_t header;
    uint32_t firmwareControl;
    uint32_t dsdt;
    uint8_t reserved;
    uint8_t preferredPMProfile;
    uint16_t sciInterrupt;
    uint32_t smiCommandPort;
    uint8_t acpiEnable;
    uint8_t acpiDisable;
    uint8_t s4BiosReq;
    uint8_t pStateCnt;
    uint32_t pm1aEventBlk;
    uint32_t pm1bEventBlk;
    uint32_t pm1aControlBlk;
    uint32_t pm1bControlBlk;
    uint32_t pm2ControlBlk;
    uint32_t pmTimerBlk;
    uint32_t gpe0Blk;
    uint32_t gpe1Blk;
    uint8_t pm1EventLength;
    uint8_t pm1ControlLength;
    uint8_t pm2ControlLength;
    uint8_t pmTimerLength;
    uint8_t gpe0BlkLength;
    uint8_t gpe1BlkLength;
    uint8_t gpe1Base;
    uint8_t cStateControl;
    uint16_t worstC2Latency;
    uint16_t worstC3Latency;
    uint16_t flushSize;
    uint16_t flushStride;
    uint8_t dutyOffset;
    uint8_t dutyWidth;
    uint8_t dayAlrm;
    uint8_t monAlrm;
    uint8_t century;
    uint16_t bootArchFlags;
    uint8_t reserved2;
    uint32_t flags;
    uint8_t resetReg[12];
    uint8_t resetValue;
    uint16_t armBootArchFlags;
    uint8_t fadtMinorVersion;
    uint64_t xFirmwareControl;
    uint64_t xDsdt;
    uint8_t xPm1aEventBlk[12];
    uint8_t xPm1bEventBlk[12];
    uint8_t xPm1aControlBlk[12];
    uint8_t xPm1bControlBlk[12];
    uint8_t xPm2ControlBlk[12];
    uint8_t xPmTimerBlk[12];
    uint8_t xGpe0Blk[12];
    uint8_t xGpe1Blk[12];
    uint8_t sleepControlReg[12];
    uint8_t sleepStatusReg[12];
    uint64_t hypervisorVendorId;
} __attribute__((__packed__)) acpi_fadt_t;

extern uintptr_t acpi_lapic_base;
extern uintptr_t acpi_ioapic_base;
int acpi_init();

extern struct madt_iso isos[32];
extern int isos_num;

void acpi_irq_handler(registers_t* regs);
int shutdown();