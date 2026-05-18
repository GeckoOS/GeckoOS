//Ember2819

#ifndef ATA_H
#define ATA_H

#include <stdint.h>

#define ATA_PRIMARY_BASE    0x1F0

#define ATA_PRIMARY_CTRL    0x3F6

#define ATA_REG_DATA        0x00
#define ATA_REG_ERROR       0x01
#define ATA_REG_FEATURES    0x01
#define ATA_REG_SECCOUNT    0x02
#define ATA_REG_LBA_LO      0x03
#define ATA_REG_LBA_MID     0x04
#define ATA_REG_LBA_HI      0x05
#define ATA_REG_HDDEVSEL    0x06
#define ATA_REG_STATUS      0x07
#define ATA_REG_COMMAND     0x07

#define ATA_SELECT_MASTER   0xA0
#define ATA_SELECT_SLAVE    0xB0
#define ATA_LBA_MASTER      0xE0
#define ATA_LBA_SLAVE       0xF0

// Status bits
#define ATA_SR_BSY          0x80
#define ATA_SR_DRDY         0x40
#define ATA_SR_DRQ          0x08
#define ATA_SR_ERR          0x01

// Commands
#define ATA_CMD_IDENTIFY    0xEC
#define ATA_CMD_READ_PIO    0x20
#define ATA_CMD_WRITE_PIO   0x30
#define ATA_CMD_FLUSH       0xE7

#define ATA_SECTOR_SIZE     512
#define ATA_DRIVE_MASTER    0
#define ATA_DRIVE_SLAVE     1

// Initialize ATA
int ata_init(void);

int ata_drive_present(int drive);

int ata_read_sectors(int drive, uint32_t lba, uint8_t count, uint8_t *buf);
int ata_write_sectors(int drive, uint32_t lba, uint8_t count, const uint8_t *buf);

#endif // ATA_H
