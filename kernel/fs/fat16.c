//Ember2819

#include "fat16.h"
#include "../drivers/ata.h"
#include "../mem.h"
#include "../terminal/terminal.h"
#include "../colors.h"
#include <stdint.h>

static FAT16_Volume vol;

static uint8_t sector_buf[ATA_SECTOR_SIZE];

// ---- Internal utilities ----

static int read_sector(uint32_t lba_offset) {
    return ata_read_sectors(vol.ata_drive, vol.partition_lba + lba_offset, 1, sector_buf);
}

static uint16_t fat16_next_cluster(uint16_t cluster) {
    // Each FAT16 entry is 2 bytes. Calculate which sector of the FAT holds it.
    uint32_t fat_offset  = cluster * 2;
    uint32_t fat_sector  = vol.fat_lba + (fat_offset / ATA_SECTOR_SIZE);
    uint32_t byte_offset = fat_offset % ATA_SECTOR_SIZE;

    if (ata_read_sectors(vol.ata_drive, fat_sector, 1, sector_buf) != 0) return 0xFFFF;

    uint16_t entry = *(uint16_t *)(sector_buf + byte_offset);
    return entry;
}

static uint32_t cluster_to_lba(uint16_t cluster) {
    return vol.data_lba + ((uint32_t)(cluster - 2) * vol.bpb.sectors_per_cluster);
}

// ---- 8.3 filename helpers ----
static void format_83_name(const uint8_t *raw_name, const uint8_t *raw_ext, char *buf) {
    int i = 0, j = 0;

    for (int n = 0; n < 8; n++) {
        if (raw_name[n] == ' ') break;
        buf[i++] = raw_name[n];
    }

    int has_ext = 0;
    for (int n = 0; n < 3; n++) {
        if (raw_ext[n] != ' ') { has_ext = 1; break; }
    }
    if (has_ext) {
        buf[i++] = '.';
        for (int n = 0; n < 3; n++) {
            if (raw_ext[n] == ' ') break;
            buf[i++] = raw_ext[n];
        }
    }

    buf[i] = '\0';
    (void)j; // unused
}

static int match_filename(const char *input, const FAT16_DirEntry *entry) {
    char entry_name[FAT16_MAX_FILENAME];
    format_83_name(entry->name, entry->ext, entry_name);

    int i = 0;
    while (input[i] && entry_name[i]) {
        char a = input[i];
        char b = entry_name[i];
        // Lowercase a
        if (a >= 'a' && a <= 'z') a -= 32;
        // Lowercase b
        if (b >= 'a' && b <= 'z') b -= 32;
        if (a != b) return 0;
        i++;
    }
    return input[i] == '\0' && entry_name[i] == '\0';
}

int fat16_mount(int drive, uint32_t partition_lba) {
    memset(&vol, 0, sizeof(FAT16_Volume));
    vol.partition_lba = partition_lba;
    vol.ata_drive = drive;

    if (ata_read_sectors(drive, partition_lba, 1, sector_buf) != 0) {
        printf("[FAT16] Error: could not read boot sector\n", VGA_COLOR_RED);
        return -1;
    }

    memcpy(&vol.bpb, sector_buf, sizeof(FAT16_BPB));
    //spec says dont do this but im doing it anyway lol
    if (vol.bpb.bytes_per_sector == 0 || vol.bpb.sectors_per_cluster == 0) {
        printf("[FAT16] Error: invalid BPB (zero bytes/sector or sectors/cluster)\n", VGA_COLOR_RED);
        return -1;
    }

    vol.fat_lba = partition_lba + vol.bpb.reserved_sectors;

    vol.root_dir_lba = vol.fat_lba
                     + ((uint32_t)vol.bpb.num_fats * vol.bpb.sectors_per_fat);

    uint32_t root_dir_sectors = ((uint32_t)vol.bpb.root_entry_count * 32
                                  + vol.bpb.bytes_per_sector - 1)
                                 / vol.bpb.bytes_per_sector;

    vol.data_lba = vol.root_dir_lba + root_dir_sectors;

    vol.total_sectors = vol.bpb.total_sectors_16 != 0
                      ? vol.bpb.total_sectors_16
                      : vol.bpb.total_sectors_32;

    vol.mounted = 1;
    return 0;
}

void fat16_list_root(void) {
    if (!vol.mounted) {
        printf("[FAT16] Not mounted.\n", VGA_COLOR_RED);
        return;
    }

    uint32_t root_dir_sectors = ((uint32_t)vol.bpb.root_entry_count * 32
                                  + vol.bpb.bytes_per_sector - 1)
                                 / vol.bpb.bytes_per_sector;

    printf("\n-- Root Directory --\n", TERM_COLOR);

    int found_any = 0;

    for (uint32_t sec = 0; sec < root_dir_sectors; sec++) {
        if (ata_read_sectors(vol.ata_drive, vol.root_dir_lba + sec, 1, sector_buf) != 0) break;

        uint32_t entries_per_sector = vol.bpb.bytes_per_sector / 32;
        FAT16_DirEntry *entries = (FAT16_DirEntry *)sector_buf;

        for (uint32_t i = 0; i < entries_per_sector; i++) {
            uint8_t first = entries[i].name[0];

            if (first == FAT16_ENTRY_END) goto done_listing;
            if (first == FAT16_ENTRY_FREE) continue;
            if (entries[i].attributes & FAT16_ATTR_VOLUME_ID) continue;
            if ((entries[i].attributes & FAT16_ATTR_LFN) == FAT16_ATTR_LFN) continue;

            char name_buf[FAT16_MAX_FILENAME];
            format_83_name(entries[i].name, entries[i].ext, name_buf);

            if (entries[i].attributes & FAT16_ATTR_DIRECTORY) {
                printf("  [DIR]  ", TERM_COLOR);
            } else {
                printf("  [FILE] ", TERM_COLOR);
            }
            printf(name_buf, TERM_COLOR);

            if (!(entries[i].attributes & FAT16_ATTR_DIRECTORY)) {
                printf("  (", TERM_COLOR);
                print_int(entries[i].file_size);
                printf(" bytes)\n", TERM_COLOR);
            } else {
                printf("\n", TERM_COLOR);
            }
            found_any = 1;
        }
    }

done_listing:
    if (!found_any) {
        printf("  (empty)\n", TERM_COLOR);
    }
    printf("--------------------------\n", TERM_COLOR);
}

int fat16_open(const char *filename, FAT16_File *file) {
    if (!vol.mounted || !filename || !file) return -1;

    memset(file, 0, sizeof(FAT16_File));

    uint32_t root_dir_sectors = ((uint32_t)vol.bpb.root_entry_count * 32
                                  + vol.bpb.bytes_per_sector - 1)
                                 / vol.bpb.bytes_per_sector;

    for (uint32_t sec = 0; sec < root_dir_sectors; sec++) {
        if (ata_read_sectors(vol.ata_drive, vol.root_dir_lba + sec, 1, sector_buf) != 0) break;

        uint32_t entries_per_sector = vol.bpb.bytes_per_sector / 32;
        FAT16_DirEntry *entries = (FAT16_DirEntry *)sector_buf;

        for (uint32_t i = 0; i < entries_per_sector; i++) {
            uint8_t first = entries[i].name[0];
            if (first == FAT16_ENTRY_END) return -1;
            if (first == FAT16_ENTRY_FREE) continue;
            if (entries[i].attributes & FAT16_ATTR_VOLUME_ID) continue;
            if ((entries[i].attributes & FAT16_ATTR_LFN) == FAT16_ATTR_LFN) continue;
            if (entries[i].attributes & FAT16_ATTR_DIRECTORY) continue;

            if (match_filename(filename, &entries[i])) {
                file->file_size      = entries[i].file_size;
                file->first_cluster  = entries[i].first_cluster;
                file->current_cluster = entries[i].first_cluster;
                file->position       = 0;
                file->cluster_offset = 0;
                file->valid          = 1;
                return 0;
            }
        }
    }

    return -1; // Not found
}

int fat16_read(FAT16_File *file, uint8_t *buf, uint32_t len) {
    if (!file || !file->valid || !buf) return -1;
    if (file->position >= file->file_size) return 0; // EOF

    uint32_t remaining = file->file_size - file->position;
    if (len > remaining) len = remaining;

    uint32_t bytes_read = 0;
    uint32_t cluster_size = (uint32_t)vol.bpb.sectors_per_cluster * ATA_SECTOR_SIZE;

    while (bytes_read < len) {
        if (file->current_cluster >= FAT16_CLUSTER_EOC ||
            file->current_cluster == FAT16_CLUSTER_BAD) break;

        // How many bytes are left in the current cluster?
        uint32_t bytes_in_cluster = cluster_size - file->cluster_offset;
        uint32_t to_copy = len - bytes_read;
        if (to_copy > bytes_in_cluster) to_copy = bytes_in_cluster;

        // Read the cluster sector by sector
        uint32_t lba = cluster_to_lba(file->current_cluster);
        uint32_t offset_in_cluster = file->cluster_offset;

        // We'll read sector by sector within the cluster
        while (to_copy > 0) {
            uint32_t sector_index   = offset_in_cluster / ATA_SECTOR_SIZE;
            uint32_t offset_in_sec  = offset_in_cluster % ATA_SECTOR_SIZE;
            uint32_t avail_in_sec   = ATA_SECTOR_SIZE - offset_in_sec;
            uint32_t chunk          = to_copy < avail_in_sec ? to_copy : avail_in_sec;

            if (ata_read_sectors(vol.ata_drive, lba + sector_index, 1, sector_buf) != 0)
                return bytes_read > 0 ? (int)bytes_read : -1;

            memcpy(buf + bytes_read, sector_buf + offset_in_sec, chunk);

            bytes_read        += chunk;
            offset_in_cluster += chunk;
            to_copy           -= chunk;
        }

        file->cluster_offset += (cluster_size - bytes_in_cluster) + (cluster_size - bytes_in_cluster == 0 ? (bytes_read < len ? cluster_size : file->cluster_offset) : 0);

        // Recalculate cleanly
        file->cluster_offset = offset_in_cluster;

        if (file->cluster_offset >= cluster_size) {
            file->cluster_offset = 0;
            file->current_cluster = fat16_next_cluster(file->current_cluster);
        }
    }

    file->position += bytes_read;
    return (int)bytes_read;
}

void fat16_close(FAT16_File *file) {
    if (file) {
        memset(file, 0, sizeof(FAT16_File));
    }
}

const FAT16_Volume *fat16_get_volume(void) {
    return &vol;
}
