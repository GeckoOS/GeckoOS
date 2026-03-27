//Ember2819
#include "ata.h"
#include "../ports.h"
#include <stdint.h>

static int drive_present[2] = {0, 0};

static void ata_delay(void) {
    inb(ATA_PRIMARY_CTRL);
    inb(ATA_PRIMARY_CTRL);
    inb(ATA_PRIMARY_CTRL);
    inb(ATA_PRIMARY_CTRL);
}

static int ata_poll_bsy(void) {
    ata_delay();
    uint32_t timeout = 100000;
    while (timeout--) {
        uint8_t status = inb(ATA_PRIMARY_BASE + ATA_REG_STATUS);
        if (status & ATA_SR_ERR)   return -1;
        if (!(status & ATA_SR_BSY)) return 0;
    }
    return -1;
}

static int ata_poll_drq(void) {
    uint32_t timeout = 100000;
    while (timeout--) {
        uint8_t status = inb(ATA_PRIMARY_BASE + ATA_REG_STATUS);
        if (status & ATA_SR_ERR) return -1;
        if (status & ATA_SR_DRQ) return 0;
    }
    return -1;
}

static int ata_check_drive(int drive) {
    uint8_t select = drive ? ATA_SELECT_SLAVE : ATA_SELECT_MASTER;

    // Select the drive
    outb(ATA_PRIMARY_BASE + ATA_REG_HDDEVSEL, select);
    ata_delay();

    outb(ATA_PRIMARY_BASE + ATA_REG_SECCOUNT, 0);
    outb(ATA_PRIMARY_BASE + ATA_REG_LBA_LO,   0);
    outb(ATA_PRIMARY_BASE + ATA_REG_LBA_MID,  0);
    outb(ATA_PRIMARY_BASE + ATA_REG_LBA_HI,   0);

    outb(ATA_PRIMARY_BASE + ATA_REG_COMMAND, ATA_CMD_IDENTIFY);
    ata_delay();

    uint8_t status = inb(ATA_PRIMARY_BASE + ATA_REG_STATUS);
    if (status == 0) return 0;

    if (ata_poll_bsy() != 0) return 0;

    uint8_t mid = inb(ATA_PRIMARY_BASE + ATA_REG_LBA_MID);
    uint8_t hi  = inb(ATA_PRIMARY_BASE + ATA_REG_LBA_HI);
    if (mid != 0 || hi != 0) return 0; 

    if (ata_poll_drq() != 0) return 0;

    for (int i = 0; i < 256; i++) {
        inw(ATA_PRIMARY_BASE + ATA_REG_DATA);
    }

    return 1;
}

// If you read this open up an issue with the secret code "SuperSecretCode12345" For a prize...

int ata_init(void) {
    int found = 0;
    drive_present[ATA_DRIVE_MASTER] = ata_check_drive(ATA_DRIVE_MASTER);
    drive_present[ATA_DRIVE_SLAVE]  = ata_check_drive(ATA_DRIVE_SLAVE);
    if (drive_present[0]) found++;
    if (drive_present[1]) found++;
    return found;
}

int ata_drive_present(int drive) {
    if (drive < 0 || drive > 1) return 0;
    return drive_present[drive];
}

int ata_read_sectors(int drive, uint32_t lba, uint8_t count, uint8_t *buf) {
    if (!drive_present[drive]) return -1;
    if (ata_poll_bsy() != 0) return -1;

    uint8_t lba_select = (drive ? ATA_LBA_SLAVE : ATA_LBA_MASTER)
                         | ((lba >> 24) & 0x0F);

    outb(ATA_PRIMARY_BASE + ATA_REG_HDDEVSEL, lba_select);
    ata_delay();
    outb(ATA_PRIMARY_BASE + ATA_REG_SECCOUNT, count);
    outb(ATA_PRIMARY_BASE + ATA_REG_LBA_LO,   (uint8_t)(lba));
    outb(ATA_PRIMARY_BASE + ATA_REG_LBA_MID,  (uint8_t)(lba >> 8));
    outb(ATA_PRIMARY_BASE + ATA_REG_LBA_HI,   (uint8_t)(lba >> 16));
    outb(ATA_PRIMARY_BASE + ATA_REG_COMMAND,   ATA_CMD_READ_PIO);

    uint16_t *p = (uint16_t *)buf;
    for (int s = 0; s < count; s++) {
        if (ata_poll_bsy() != 0) return -1;
        if (ata_poll_drq() != 0) return -1;
        for (int i = 0; i < 256; i++) {
            p[s * 256 + i] = inw(ATA_PRIMARY_BASE + ATA_REG_DATA);
        }
    }
    return 0;
}

int ata_write_sectors(int drive, uint32_t lba, uint8_t count, const uint8_t *buf) {
    if (!drive_present[drive]) return -1;
    if (ata_poll_bsy() != 0) return -1;

    uint8_t lba_select = (drive ? ATA_LBA_SLAVE : ATA_LBA_MASTER)
                         | ((lba >> 24) & 0x0F);

    outb(ATA_PRIMARY_BASE + ATA_REG_HDDEVSEL, lba_select);
    ata_delay();
    outb(ATA_PRIMARY_BASE + ATA_REG_SECCOUNT, count);
    outb(ATA_PRIMARY_BASE + ATA_REG_LBA_LO,   (uint8_t)(lba));
    outb(ATA_PRIMARY_BASE + ATA_REG_LBA_MID,  (uint8_t)(lba >> 8));
    outb(ATA_PRIMARY_BASE + ATA_REG_LBA_HI,   (uint8_t)(lba >> 16));
    outb(ATA_PRIMARY_BASE + ATA_REG_COMMAND,   ATA_CMD_WRITE_PIO);

    const uint16_t *p = (const uint16_t *)buf;
    for (int s = 0; s < count; s++) {
        if (ata_poll_bsy() != 0) return -1;
        if (ata_poll_drq() != 0) return -1;
        for (int i = 0; i < 256; i++) {
            outw(ATA_PRIMARY_BASE + ATA_REG_DATA, p[s * 256 + i]);
        }
        outb(ATA_PRIMARY_BASE + ATA_REG_COMMAND, ATA_CMD_FLUSH);
        if (ata_poll_bsy() != 0) return -1;
    }
    return 0;
}
