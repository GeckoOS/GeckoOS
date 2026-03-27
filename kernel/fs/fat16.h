//Ember2819
//FAT16 driver for GeckoOS
#ifndef FAT16_H
#define FAT16_H

#include <stdint.h>

typedef struct __attribute__((packed)) {
    uint8_t  jmp_boot[3];        // Jump instruction + NOP
    uint8_t  oem_name[8];        // OEM name
    uint16_t bytes_per_sector;   // Almost always 512 just wait soon enough it wont be...
    uint8_t  sectors_per_cluster;
    uint16_t reserved_sectors;
    uint8_t  num_fats;
    uint16_t root_entry_count;   // Max number of root directory entries
    uint16_t total_sectors_16;
    uint8_t  media_type;
    uint16_t sectors_per_fat;
    uint16_t sectors_per_track;
    uint16_t num_heads;
    uint32_t hidden_sectors;     // Sectors before this partition
    uint32_t total_sectors_32;

    uint8_t  drive_number;
    uint8_t  reserved1;
    uint8_t  boot_signature;
    uint32_t volume_id;          // Serial number
    uint8_t  volume_label[11];
    uint8_t  fs_type[8];
} FAT16_BPB;

// A single 32-byte directory entry.
typedef struct __attribute__((packed)) {
    uint8_t  name[8];            // Filename 
    uint8_t  ext[3];             // Extension 
    uint8_t  attributes;         // File attribute flags 
    uint8_t  reserved;
    uint8_t  create_time_tenth;  // Creation time
    uint16_t create_time;        // Creation time also
    uint16_t create_date;        // Creation date 
    uint16_t access_date;        // Last access date
    uint16_t first_cluster_high;
    uint16_t write_time;         // Last write time
    uint16_t write_date;         // Last write date
    uint16_t first_cluster;
    uint32_t file_size;          // File size in bytes
} FAT16_DirEntry;

#define FAT16_ATTR_READ_ONLY  0x01
#define FAT16_ATTR_HIDDEN     0x02
#define FAT16_ATTR_SYSTEM     0x04
#define FAT16_ATTR_VOLUME_ID  0x08
#define FAT16_ATTR_DIRECTORY  0x10
#define FAT16_ATTR_ARCHIVE    0x20
#define FAT16_ATTR_LFN        0x0F

#define FAT16_ENTRY_FREE      0xE5  // Slot is free (deleted)
#define FAT16_ENTRY_END       0x00  // No more entries after this

#define FAT16_CLUSTER_FREE    0x0000
#define FAT16_CLUSTER_BAD     0xFFF7
#define FAT16_CLUSTER_EOC     0xFFF8  // End of chain (0xFFF8–0xFFFF)

#define FAT16_MAX_FILENAME    13

typedef struct {
    FAT16_BPB bpb;

    uint32_t  partition_lba;
    uint32_t  fat_lba;
    uint32_t  root_dir_lba;
    uint32_t  data_lba;
    uint32_t  total_sectors;
    uint8_t   mounted;
    int       ata_drive;
} FAT16_Volume;

typedef struct {
    uint32_t  file_size;         // Total file size in bytes
    uint32_t  position;
    uint16_t  first_cluster;     // Starting cluster of the file
    uint16_t  current_cluster;   // Cluster currently being read
    uint32_t  cluster_offset;    // Byte offset within the current cluster
    uint8_t   valid;
} FAT16_File;

int fat16_mount(int drive, uint32_t partition_lba);

void fat16_list_root(void);

int fat16_open(const char *filename, FAT16_File *file);

int fat16_read(FAT16_File *file, uint8_t *buf, uint32_t len);

void fat16_close(FAT16_File *file);

const FAT16_Volume *fat16_get_volume(void);

#endif // FAT16_H
