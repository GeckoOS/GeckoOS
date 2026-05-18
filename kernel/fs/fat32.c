// Code created by Ember2819, with assistance from Claude Sonnet 4.6.
// All code was manually reviewed and tested.
// Scope of AI usage:
// Help with transforming FAT16 to FAT32 and help with debugging


#include <fs/fat32.h>
#include <drivers/ata.h>
#include <mem.h>
#include <terminal/terminal.h>
#include <colors.h>
#include <partition/partition.h>
#include <fs/fs.h>
#include <stdint.h>

typedef struct __attribute__((packed)) {
    uint8_t  jmp_boot[3];
    uint8_t  oem_name[8];
    uint16_t bytes_per_sector;
    uint8_t  sectors_per_cluster;
    uint16_t reserved_sectors;
    uint8_t  num_fats;
    uint16_t root_entry_count;   // Must be 0 for FAT32
    uint16_t total_sectors_16;   // Must be 0 for FAT32
    uint8_t  media_type;
    uint16_t sectors_per_fat_16; // Must be 0 for FAT32
    uint16_t sectors_per_track;
    uint16_t num_heads;
    uint32_t hidden_sectors;
    uint32_t total_sectors_32;

    // FAT32 extended fields
    uint32_t sectors_per_fat_32;
    uint16_t ext_flags;
    uint16_t fs_version;
    uint32_t root_cluster;       // Usually 2
    uint16_t fs_info_sector;
    uint16_t backup_boot_sector;
    uint8_t  reserved[12];
    uint8_t  drive_number;
    uint8_t  reserved1;
    uint8_t  boot_signature;
    uint32_t volume_id;
    uint8_t  volume_label[11];
    uint8_t  fs_type[8];         // "FAT32   "
} FAT32_BPB;

typedef struct __attribute__((packed)) {
    uint8_t  name[8];
    uint8_t  ext[3];
    uint8_t  attributes;
    uint8_t  reserved;
    uint8_t  create_time_tenth;
    uint16_t create_time;
    uint16_t create_date;
    uint16_t access_date;
    uint16_t first_cluster_high;
    uint16_t write_time;
    uint16_t write_date;
    uint16_t first_cluster_low;
    uint32_t file_size;
} FAT32_DirEntry;

#define FAT32_ATTR_READ_ONLY  0x01
#define FAT32_ATTR_HIDDEN     0x02
#define FAT32_ATTR_SYSTEM     0x04
#define FAT32_ATTR_VOLUME_ID  0x08
#define FAT32_ATTR_DIRECTORY  0x10
#define FAT32_ATTR_ARCHIVE    0x20
#define FAT32_ATTR_LFN        0x0F

#define FAT32_ENTRY_FREE      0xE5
#define FAT32_ENTRY_END       0x00

#define FAT32_CLUSTER_FREE    0x00000000UL
#define FAT32_CLUSTER_BAD     0x0FFFFFF7UL
#define FAT32_CLUSTER_EOC     0x0FFFFFF8UL  // 0x0FFFFFF8 – 0x0FFFFFFF = EOC
#define FAT32_CLUSTER_MASK    0x0FFFFFFFUL

#define FAT32_MAX_FILENAME    13

typedef struct {
    FAT32_BPB bpb;

    uint32_t  partition_lba;
    uint32_t  fat_lba;
    uint32_t  data_lba;       // FAT32 has no fixed root-dir region
    uint32_t  root_cluster;   // First cluster of root directory
    uint32_t  total_sectors;
    uint32_t  sectors_per_fat;
    uint8_t   mounted;
    struct kdrive_t *drive;
} FAT32_Volume;

static uint8_t sector_buf[ATA_SECTOR_SIZE];

static uint32_t dir_entry_cluster(const FAT32_DirEntry *e) {
    return ((uint32_t)e->first_cluster_high << 16) | e->first_cluster_low;
}

static uint32_t cluster_to_lba(FAT32_Volume *pvol, uint32_t cluster) {
    return pvol->data_lba + ((cluster - 2) * pvol->bpb.sectors_per_cluster);
}

static uint32_t fat32_next_cluster(struct kdrive_t *drive, FAT32_Volume *pvol,
                                   uint32_t cluster)
{
    uint32_t fat_offset  = cluster * 4;
    uint32_t fat_sector  = pvol->fat_lba + (fat_offset / drive->sector_size);
    uint32_t byte_offset = fat_offset % drive->sector_size;

    if (drive->read((void*)drive, fat_sector, 1, sector_buf) < 0)
        return FAT32_CLUSTER_BAD;

    uint32_t entry = *(uint32_t *)(sector_buf + byte_offset) & FAT32_CLUSTER_MASK;
    return entry;
}

static int fat32_write_fat(struct kdrive_t *drive, FAT32_Volume *pvol,
                           uint32_t cluster, uint32_t value)
{
    uint32_t fat_offset  = cluster * 4;
    uint32_t fat_sector  = pvol->fat_lba + (fat_offset / drive->sector_size);
    uint32_t byte_offset = fat_offset % drive->sector_size;
    uint8_t  tmp[ATA_SECTOR_SIZE];

    if (drive->read((void*)drive, fat_sector, 1, tmp) < 0) return -1;

    // Preserve top 4 reserved bits of existing entry, write only 28 value bits
    uint32_t existing = *(uint32_t *)(tmp + byte_offset);
    uint32_t updated  = (existing & 0xF0000000UL) | (value & FAT32_CLUSTER_MASK);
    *(uint32_t *)(tmp + byte_offset) = updated;

    if (drive->write((void*)drive, fat_sector, 1, tmp) < 0) return -1;

    // Mirror to FAT2 if present
    if (pvol->bpb.num_fats > 1) {
        uint32_t fat2_sector = fat_sector + pvol->sectors_per_fat;
        if (drive->write((void*)drive, fat2_sector, 1, tmp) < 0) return -1;
    }
    return 0;
}

static uint32_t fat32_alloc_cluster(struct kdrive_t *drive, FAT32_Volume *pvol)
{
    uint32_t fat_sectors      = pvol->sectors_per_fat;
    uint32_t entries_per_sector = drive->sector_size / 4;
    uint8_t  tmp[ATA_SECTOR_SIZE];

    for (uint32_t s = 0; s < fat_sectors; s++) {
        if (drive->read((void*)drive, pvol->fat_lba + s, 1, tmp) < 0) return 0;
        for (uint32_t e = 0; e < entries_per_sector; e++) {
            uint32_t val     = *(uint32_t *)(tmp + e * 4) & FAT32_CLUSTER_MASK;
            uint32_t cluster = s * entries_per_sector + e;
            if (cluster < 2) continue;
            if (val == FAT32_CLUSTER_FREE) {
                // Mark as EOC, preserving top nibble
                uint32_t existing = *(uint32_t *)(tmp + e * 4);
                *(uint32_t *)(tmp + e * 4) =
                    (existing & 0xF0000000UL) | (FAT32_CLUSTER_EOC & FAT32_CLUSTER_MASK);
                if (drive->write((void*)drive, pvol->fat_lba + s, 1, tmp) < 0) return 0;
                // Mirror to FAT2
                if (pvol->bpb.num_fats > 1)
                    drive->write((void*)drive,
                                 pvol->fat_lba + pvol->sectors_per_fat + s, 1, tmp);
                return cluster;
            }
        }
    }
    return 0; // disk full
}

static void format_83_name(const uint8_t *raw_name, const uint8_t *raw_ext,
                            char *buf)
{
    int i = 0;
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
}

static void encode_83_name(char *name, uint8_t *raw8, uint8_t *raw3)
{
    int i;
    for (i = 0; i < 8; i++) raw8[i] = ' ';
    for (i = 0; i < 3; i++) raw3[i] = ' ';

    int dot = -1;
    for (i = 0; name[i]; i++) if (name[i] == '.') dot = i;

    int base_len = (dot >= 0) ? dot : (int)strlen(name);
    if (base_len > 8) base_len = 8;
    for (i = 0; i < base_len; i++) {
        char c = name[i];
        if (c >= 'a' && c <= 'z') c -= 32;
        raw8[i] = (uint8_t)c;
    }
    if (dot >= 0) {
        int ext_len = (int)strlen(name) - dot - 1;
        if (ext_len > 3) ext_len = 3;
        for (i = 0; i < ext_len; i++) {
            char c = name[dot + 1 + i];
            if (c >= 'a' && c <= 'z') c -= 32;
            raw3[i] = (uint8_t)c;
        }
    }
}

typedef int (*dir_entry_cb)(FAT32_DirEntry *entry, uint32_t lba,
                             uint32_t entry_index_in_sector, void *ctx);

static void fat32_dir_iterate(struct kdrive_t *drive, FAT32_Volume *pvol,
                               uint32_t start_cluster,
                               uint8_t *buf,
                               dir_entry_cb cb, void *ctx)
{
    uint32_t cluster = start_cluster;
    uint32_t entries_per_sector = drive->sector_size / 32;

    while (cluster >= 2 && cluster < FAT32_CLUSTER_BAD) {
        uint32_t lba = cluster_to_lba(pvol, cluster);
        for (uint32_t s = 0; s < pvol->bpb.sectors_per_cluster; s++) {
            if (drive->read((void*)drive, lba + s, 1, buf) < 0) return;
            FAT32_DirEntry *entries = (FAT32_DirEntry *)buf;
            for (uint32_t e = 0; e < entries_per_sector; e++) {
                if (cb(&entries[e], lba + s, e, ctx)) return;
            }
        }
        cluster = fat32_next_cluster(drive, pvol, cluster);
    }
}

static size_t fat32_file_read(struct drive_file_t *file, size_t offset,
                               size_t len, uint8_t *buf)
{
    if (offset >= file->file_size) return 0;
    size_t remaining = file->file_size - offset;
    if (len > remaining) len = remaining;

    FAT32_Volume *pvol   = (FAT32_Volume *)file->fs->userdata1;
    size_t cluster_size  = (size_t)pvol->bpb.bytes_per_sector *
                           pvol->bpb.sectors_per_cluster;
    uint32_t current_cluster = (uint32_t)file->userdata2;

    // Skip clusters to reach the requested offset
    uint32_t clusters_to_skip = (uint32_t)(offset / cluster_size);
    size_t   cluster_offset   = offset % cluster_size;
    for (uint32_t i = 0; i < clusters_to_skip; i++) {
        current_cluster = fat32_next_cluster(file->drive, pvol, current_cluster);
        if (current_cluster >= FAT32_CLUSTER_BAD) return 0;
    }

    size_t bytes_read = 0;
    while (bytes_read < len) {
        if (current_cluster < 2 || current_cluster >= FAT32_CLUSTER_BAD) break;

        uint32_t lba              = cluster_to_lba(pvol, current_cluster);
        size_t   offset_in_cluster = cluster_offset;
        int32_t  bytes_in_cluster  = (int32_t)(cluster_size - cluster_offset);
        uint32_t to_copy = (uint32_t)(len - bytes_read);
        if ((int32_t)to_copy > bytes_in_cluster)
            to_copy = (uint32_t)bytes_in_cluster;

        while (to_copy > 0) {
            uint32_t sector_index  = (uint32_t)(offset_in_cluster / file->drive->sector_size);
            uint32_t offset_in_sec = (uint32_t)(offset_in_cluster % file->drive->sector_size);
            uint32_t avail         = file->drive->sector_size - offset_in_sec;
            uint32_t chunk         = to_copy < avail ? to_copy : avail;

            if (file->drive->read((void*)file->drive, lba + sector_index, 1,
                                   sector_buf) < 0)
                return bytes_read > 0 ? bytes_read : (size_t)-1;

            memcpy(buf + bytes_read, sector_buf + offset_in_sec, chunk);
            bytes_read        += chunk;
            offset_in_cluster += chunk;
            to_copy           -= chunk;
        }

        cluster_offset = offset_in_cluster;
        if (cluster_offset >= cluster_size) {
            cluster_offset  = 0;
            current_cluster = fat32_next_cluster(file->drive, pvol, current_cluster);
        }
    }
    return bytes_read;
}

static size_t fat32_file_write(struct drive_file_t *file, size_t offset,
                                size_t len, const uint8_t *buf)
{
    FAT32_Volume *pvol    = (FAT32_Volume *)file->fs->userdata1;
    size_t cluster_size   = (size_t)pvol->bpb.bytes_per_sector *
                            pvol->bpb.sectors_per_cluster;
    uint32_t current_cluster = (uint32_t)file->userdata2;
    size_t bytes_written  = 0;

    if (offset >= file->file_size) return 0;
    if (offset + len > file->file_size) len = file->file_size - offset;

    uint32_t clusters_to_skip = (uint32_t)(offset / cluster_size);
    size_t   cluster_offset   = offset % cluster_size;

    for (uint32_t i = 0; i < clusters_to_skip; i++) {
        current_cluster = fat32_next_cluster(file->drive, pvol, current_cluster);
        if (current_cluster >= FAT32_CLUSTER_BAD) return bytes_written;
    }

    uint8_t tmp[ATA_SECTOR_SIZE];
    while (bytes_written < len) {
        if (current_cluster < 2 || current_cluster >= FAT32_CLUSTER_BAD) break;

        uint32_t lba              = cluster_to_lba(pvol, current_cluster);
        size_t   offset_in_cluster = cluster_offset;

        while (bytes_written < len && offset_in_cluster < cluster_size) {
            uint32_t sector_index  = (uint32_t)(offset_in_cluster / file->drive->sector_size);
            uint32_t offset_in_sec = (uint32_t)(offset_in_cluster % file->drive->sector_size);
            uint32_t avail         = file->drive->sector_size - offset_in_sec;
            uint32_t chunk         = (uint32_t)(len - bytes_written);
            if (chunk > avail) chunk = avail;

            if (file->drive->read((void*)file->drive, lba + sector_index, 1, tmp) < 0)
                return bytes_written;
            memcpy(tmp + offset_in_sec, buf + bytes_written, chunk);
            if (file->drive->write((void*)file->drive, lba + sector_index, 1, tmp) < 0)
                return bytes_written;

            bytes_written      += chunk;
            offset_in_cluster  += chunk;
        }
        cluster_offset = 0;
        if (bytes_written < len)
            current_cluster = fat32_next_cluster(file->drive, pvol, current_cluster);
    }
    return bytes_written;
}

static uint32_t fat32_write_cluster_chain(struct kdrive_t *drive,
                                           FAT32_Volume *pvol,
                                           const uint8_t *content, size_t len)
{
    size_t cluster_size    = (size_t)pvol->bpb.bytes_per_sector *
                             pvol->bpb.sectors_per_cluster;
    size_t clusters_needed = (len == 0) ? 1
                             : (len + cluster_size - 1) / cluster_size;

    uint32_t first_cluster = 0;
    uint32_t prev_cluster  = 0;
    size_t   remaining     = len;

    for (size_t c = 0; c < clusters_needed; c++) {
        uint32_t cl = fat32_alloc_cluster(drive, pvol);
        if (cl == 0) return 0;

        if (first_cluster == 0) first_cluster = cl;
        if (prev_cluster  != 0) fat32_write_fat(drive, pvol, prev_cluster, cl);
        prev_cluster = cl;

        uint32_t lba = cluster_to_lba(pvol, cl);
        uint8_t  tmp[ATA_SECTOR_SIZE];

        for (uint32_t s = 0; s < pvol->bpb.sectors_per_cluster; s++) {
            memset(tmp, 0, drive->sector_size);
            size_t chunk = (remaining > drive->sector_size) ? drive->sector_size
                                                             : remaining;
            if (chunk > 0 && content) {
                size_t src_off = len - remaining;
                memcpy(tmp, content + src_off, chunk);
                remaining -= chunk;
            }
            drive->write((void*)drive, lba + s, 1, tmp);
        }
    }
    return first_cluster;
}
// free a cluster chain
static void fat32_free_cluster_chain(struct kdrive_t *drive, FAT32_Volume *pvol,
                                      uint32_t cluster)
{
    while (cluster >= 2 && cluster < FAT32_CLUSTER_BAD) {
        uint32_t next = fat32_next_cluster(drive, pvol, cluster);
        fat32_write_fat(drive, pvol, cluster, FAT32_CLUSTER_FREE);
        cluster = next;
    }
}

typedef struct {
    // In
    FAT32_DirEntry *entry_to_write; // NULL = just find; non-NULL = write & stop
    uint8_t target8[8];
    uint8_t target3[3];
    int find_free;    // 1 = looking for a free slot
    int find_match;   // 1 = looking for a name match

    int found;
    uint32_t found_lba;
    uint32_t found_idx; // index within found_lba sector
    FAT32_DirEntry result; // copy of the entry found (for read operations)
} DirSearchCtx;

static int dir_search_cb(FAT32_DirEntry *e, uint32_t lba,
                          uint32_t idx, void *ctx_raw)
{
    DirSearchCtx *ctx = (DirSearchCtx *)ctx_raw;
    uint8_t first = e->name[0];

    if (ctx->find_free) {
        if (first == FAT32_ENTRY_FREE || first == FAT32_ENTRY_END) {
            ctx->found     = 1;
            ctx->found_lba = lba;
            ctx->found_idx = idx;
            return 1; // stop
        }
        return 0;
    }

    if (ctx->find_match) {
        if (first == FAT32_ENTRY_END)  return 1; // stop – no more entries
        if (first == FAT32_ENTRY_FREE) return 0;
        if (e->attributes & FAT32_ATTR_VOLUME_ID) return 0;
        if ((e->attributes & FAT32_ATTR_LFN) == FAT32_ATTR_LFN) return 0;

        int match = 1;
        for (int k = 0; k < 8; k++)
            if (e->name[k] != ctx->target8[k]) { match = 0; break; }
        if (match)
            for (int k = 0; k < 3; k++)
                if (e->ext[k] != ctx->target3[k]) { match = 0; break; }

        if (match) {
            ctx->found     = 1;
            ctx->found_lba = lba;
            ctx->found_idx = idx;
            ctx->result    = *e;
            return 1; // stop
        }
    }
    return 0;
}

// Write a modified directory entry back to disk
static int fat32_write_dir_entry(struct kdrive_t *drive,
                                  uint32_t lba, uint32_t idx_in_sector,
                                  const FAT32_DirEntry *entry)
{
    uint8_t tmp[ATA_SECTOR_SIZE];
    if (drive->read((void*)drive, lba, 1, tmp) < 0) return -1;
    FAT32_DirEntry *slot = (FAT32_DirEntry *)tmp + idx_in_sector;
    *slot = *entry;
    if (drive->write((void*)drive, lba, 1, tmp) < 0) return -1;
    return 0;
}

int fat32_create_file(struct drive_fs_t *fs, char *name,
                       const uint8_t *content, size_t len)
{
    FAT32_Volume   *pvol  = (FAT32_Volume *)fs->userdata1;
    struct kdrive_t *drive = fs->drive;

    // Find a free slot in root directory
    uint8_t tmp[ATA_SECTOR_SIZE];
    DirSearchCtx ctx;
    memset(&ctx, 0, sizeof(ctx));
    ctx.find_free = 1;

    fat32_dir_iterate(drive, pvol, pvol->root_cluster, tmp, dir_search_cb, &ctx);
    if (!ctx.found) return -1; // root directory full

    // Allocate cluster chain for content
    uint32_t first_cluster = fat32_write_cluster_chain(drive, pvol, content, len);
    if (first_cluster == 0 && len > 0) return -1;

    // Build directory entry
    FAT32_DirEntry entry;
    memset(&entry, 0, sizeof(entry));
    encode_83_name(name, entry.name, entry.ext);
    entry.attributes        = FAT32_ATTR_ARCHIVE;
    entry.first_cluster_high = (uint16_t)(first_cluster >> 16);
    entry.first_cluster_low  = (uint16_t)(first_cluster & 0xFFFF);
    entry.file_size          = (uint32_t)len;
    entry.write_date         = (uint16_t)((44 << 9) | (1 << 5) | 1);
    entry.write_time         = 0;
    entry.create_date        = entry.write_date;
    entry.create_time        = 0;

    return fat32_write_dir_entry(drive, ctx.found_lba, ctx.found_idx, &entry);
}

int fat32_write_file(struct drive_fs_t *fs, char *name,
                      const uint8_t *content, size_t len)
{
    FAT32_Volume   *pvol  = (FAT32_Volume *)fs->userdata1;
    struct kdrive_t *drive = fs->drive;

    uint8_t tmp[ATA_SECTOR_SIZE];
    DirSearchCtx ctx;
    memset(&ctx, 0, sizeof(ctx));
    ctx.find_match = 1;
    encode_83_name(name, ctx.target8, ctx.target3);

    fat32_dir_iterate(drive, pvol, pvol->root_cluster, tmp, dir_search_cb, &ctx);

    if (!ctx.found)
        return fat32_create_file(fs, name, content, len);

    // Free old cluster chain
    uint32_t old_cluster = ((uint32_t)ctx.result.first_cluster_high << 16) |
                            ctx.result.first_cluster_low;
    fat32_free_cluster_chain(drive, pvol, old_cluster);

    // Allocate new chain and write content
    uint32_t first_cluster = fat32_write_cluster_chain(drive, pvol, content, len);
    if (first_cluster == 0 && len > 0) return -1;

    // Update directory entry in place
    FAT32_DirEntry updated = ctx.result;
    updated.first_cluster_high = (uint16_t)(first_cluster >> 16);
    updated.first_cluster_low  = (uint16_t)(first_cluster & 0xFFFF);
    updated.file_size           = (uint32_t)len;
    updated.write_date          = (uint16_t)((44 << 9) | (1 << 5) | 1);
    updated.write_time          = 0;

    return fat32_write_dir_entry(drive, ctx.found_lba, ctx.found_idx, &updated);
}

int fat32_delete_file(struct drive_fs_t *fs, char *name)
{
    if (!fs) return -1;
    FAT32_Volume   *pvol  = (FAT32_Volume *)fs->userdata1;
    struct kdrive_t *drive = fs->drive;

    uint8_t tmp[ATA_SECTOR_SIZE];
    DirSearchCtx ctx;
    memset(&ctx, 0, sizeof(ctx));
    ctx.find_match = 1;
    encode_83_name(name, ctx.target8, ctx.target3);

    fat32_dir_iterate(drive, pvol, pvol->root_cluster, tmp, dir_search_cb, &ctx);
    if (!ctx.found) return -1;

    // Free cluster chain
    uint32_t cluster = ((uint32_t)ctx.result.first_cluster_high << 16) |
                        ctx.result.first_cluster_low;
    fat32_free_cluster_chain(drive, pvol, cluster);

    // Mark entry as free
    FAT32_DirEntry del = ctx.result;
    del.name[0] = FAT32_ENTRY_FREE;
    return fat32_write_dir_entry(drive, ctx.found_lba, ctx.found_idx, &del);
}

//ember2819
int fat32_append_file(struct drive_fs_t *fs, char *name,
                       const uint8_t *content, size_t len)
{
    if (!fs) return -1;

    static uint8_t appbuf[4096];
    int total = 0;

    struct fs_entries_t entries = fs->get_entries((void*)fs);
    int found = -1;
    for (int i = 0; i < (int)entries.count; i++) {
        if (entries.entries[i].type != ENTRY_FILE) continue;
        const char *a = entries.entries[i].file.name;
        const char *b = name;
        int match = 1;
        while (*a && *b) {
            char ca = (*a >= 'a' && *a <= 'z') ? *a - 32 : *a;
            char cb = (*b >= 'a' && *b <= 'z') ? *b - 32 : *b;
            if (ca != cb) { match = 0; break; }
            a++; b++;
        }
        if (match && *a == '\0' && *b == '\0') { found = i; break; }
    }

    if (found >= 0) {
        int j = 0;
        while (total < 4000) {
            uint8_t tmp[128];
            int chunk = (int)entries.entries[found].file.read(
                (void*)&entries.entries[found].file, (size_t)(j * 128), 128, tmp);
            if (chunk <= 0) break;
            for (int k = 0; k < chunk && total < 4000; k++)
                appbuf[total++] = tmp[k];
            j++;
        }
        fat32_delete_file(fs, name);
    }

    for (size_t i = 0; i < len && total < 4095; i++)
        appbuf[total++] = content[i];

    return fat32_create_file(fs, name, appbuf, (size_t)total);
}

//ember2819
int fat32_mkdir(struct drive_fs_t *fs, char *name)
{
    if (!fs) return -1;
    FAT32_Volume   *pvol  = (FAT32_Volume *)fs->userdata1;
    struct kdrive_t *drive = fs->drive;

    uint8_t tmp[ATA_SECTOR_SIZE];
    DirSearchCtx ctx;
    memset(&ctx, 0, sizeof(ctx));
    ctx.find_free = 1;

    fat32_dir_iterate(drive, pvol, pvol->root_cluster, tmp, dir_search_cb, &ctx);
    if (!ctx.found) return -1;

    FAT32_DirEntry entry;
    memset(&entry, 0, sizeof(entry));
    encode_83_name(name, entry.name, entry.ext);
    entry.attributes         = FAT32_ATTR_DIRECTORY;
    entry.first_cluster_high = 0;
    entry.first_cluster_low  = 0;
    entry.file_size          = 0;
    entry.write_date         = (uint16_t)((44 << 9) | (1 << 5) | 1);

    return fat32_write_dir_entry(drive, ctx.found_lba, ctx.found_idx, &entry);
}

void fat32_print_info(struct drive_fs_t *fs, uint8_t color)
{
    FAT32_Volume *v = (FAT32_Volume *)fs->userdata1;
    printc("-- FAT32 Volume Info --\n", color);
    printc("  Bytes/sector:      ", color); print_int(v->bpb.bytes_per_sector);    printc("\n", color);
    printc("  Sectors/cluster:   ", color); print_int(v->bpb.sectors_per_cluster); printc("\n", color);
    printc("  Reserved sectors:  ", color); print_int(v->bpb.reserved_sectors);    printc("\n", color);
    printc("  FATs:              ", color); print_int(v->bpb.num_fats);             printc("\n", color);
    printc("  Sectors/FAT:       ", color); print_int(v->sectors_per_fat);          printc("\n", color);
    printc("  Root cluster:      ", color); print_int(v->root_cluster);             printc("\n", color);
    printc("  Total sectors:     ", color); print_int(v->total_sectors);            printc("\n", color);
    printc("  FAT LBA:           ", color); print_int(v->fat_lba);                  printc("\n", color);
    printc("  Data area LBA:     ", color); print_int(v->data_lba);                 printc("\n", color);
    printc("-----------------------\n", color);
}

typedef struct {
    FAT32_Volume   *pvol;
    struct drive_fs_t *fs;
    size_t count;
    drive_entry_t *entries;
    size_t idx;
    int counting; // 1 = first pass (count), 0 = second pass (fill)
} EnumCtx;

static int enum_count_cb(FAT32_DirEntry *e, uint32_t lba,
                          uint32_t idx, void *ctx_raw)
{
    (void)lba; (void)idx;
    EnumCtx *ctx = (EnumCtx *)ctx_raw;
    uint8_t first = e->name[0];
    if (first == FAT32_ENTRY_END) return 1;
    if (first == FAT32_ENTRY_FREE) return 0;
    if (e->attributes & FAT32_ATTR_VOLUME_ID) return 0;
    if ((e->attributes & FAT32_ATTR_LFN) == FAT32_ATTR_LFN) return 0;
    ctx->count++;
    return 0;
}

static int enum_fill_cb(FAT32_DirEntry *e, uint32_t lba,
                         uint32_t idx_in_sec, void *ctx_raw)
{
    (void)lba; (void)idx_in_sec;
    EnumCtx *ctx = (EnumCtx *)ctx_raw;
    uint8_t first = e->name[0];
    if (first == FAT32_ENTRY_END) return 1;
    if (first == FAT32_ENTRY_FREE) return 0;
    if (e->attributes & FAT32_ATTR_VOLUME_ID) return 0;
    if ((e->attributes & FAT32_ATTR_LFN) == FAT32_ATTR_LFN) return 0;

    drive_entry_t *entry = &ctx->entries[ctx->idx];
    uint32_t cluster = ((uint32_t)e->first_cluster_high << 16) | e->first_cluster_low;

    if (e->attributes & FAT32_ATTR_DIRECTORY) {
        entry->type     = ENTRY_DIRECTORY;
        entry->dir.fs   = ctx->fs;
        entry->dir.drive = ctx->fs->drive;
        format_83_name(e->name, e->ext, entry->dir.name);
    } else {
        entry->type            = ENTRY_FILE;
        entry->file.fs         = ctx->fs;
        entry->file.drive      = ctx->fs->drive;
        entry->file.file_size  = e->file_size;
        entry->file.userdata1  = ctx->pvol;
        entry->file.userdata2  = (size_t)cluster;
        entry->file.read       = (fn_df_read)fat32_file_read;
        entry->file.write      = (fn_df_write)fat32_file_write;
        format_83_name(e->name, e->ext, entry->file.name);
    }
    ctx->idx++;
    return 0;
}

static struct fs_entries_t fat32_get_root_entries(struct drive_fs_t *fs)
{
    FAT32_Volume *pvol = (FAT32_Volume *)fs->userdata1;
    struct fs_entries_t result;
    result.count   = 0;
    result.entries = 0;

    uint8_t tmp[ATA_SECTOR_SIZE];
    EnumCtx ctx;
    memset(&ctx, 0, sizeof(ctx));
    ctx.pvol     = pvol;
    ctx.fs       = fs;
    ctx.counting = 1;

    fat32_dir_iterate(fs->drive, pvol, pvol->root_cluster, tmp,
                      enum_count_cb, &ctx);

    if (ctx.count == 0) return result;

    ctx.entries  = kmalloc(ctx.count * sizeof(drive_entry_t));
    memset(ctx.entries, 0, ctx.count * sizeof(drive_entry_t));
    ctx.idx      = 0;
    ctx.counting = 0;

    fat32_dir_iterate(fs->drive, pvol, pvol->root_cluster, tmp,
                      enum_fill_cb, &ctx);

    result.count   = ctx.count;
    result.entries = ctx.entries;
    return result;
}

struct drive_fs_t *fat32_drive_open(struct kdrive_t *drive,
                                     struct partition_t *partition)
{
    FAT32_Volume volume;
    memset(&volume, 0, sizeof(FAT32_Volume));
    volume.partition_lba = partition->lba;
    volume.drive         = drive;

    if (drive->read((void*)drive, partition->lba, 1, sector_buf) < 0)
        return NULL;

    memcpy(&volume.bpb, sector_buf, sizeof(FAT32_BPB));

    if (volume.bpb.bytes_per_sector == 0 || volume.bpb.sectors_per_cluster == 0) {
        kprintf(SEVERITY_ERROR,
                "FAT32] Error: invalid BPB (zero bytes/sector or sectors/cluster)\n");
        return NULL;
    }

    if (volume.bpb.sectors_per_fat_16 != 0) {
        kprintf(SEVERITY_ERROR,
                "FAT32] Error: sectors_per_fat_16 != 0; this may be a FAT16 volume\n");
        return NULL;
    }

    volume.sectors_per_fat = volume.bpb.sectors_per_fat_32;
    volume.fat_lba         = partition->lba + volume.bpb.reserved_sectors;
    volume.data_lba        = volume.fat_lba +
                             ((uint32_t)volume.bpb.num_fats * volume.sectors_per_fat);
    volume.root_cluster    = volume.bpb.root_cluster;
    volume.total_sectors   = volume.bpb.total_sectors_32; // always non-zero for FAT32
    volume.mounted         = 1;

    struct drive_fs_t *filesystem = kmalloc(sizeof(struct drive_fs_t));
    FAT32_Volume      *pvolume    = kmalloc(sizeof(FAT32_Volume));
    memcpy(pvolume, &volume, sizeof(FAT32_Volume));

    filesystem->drive       = drive;
    filesystem->userdata1   = pvolume;
    filesystem->get_entries = (fn_root_get_entries)fat32_get_root_entries;
    return filesystem;
}

struct drive_fs_t *fat32_drive_close(struct drive_fs_t *fs)
{
    (void)fs;
    return NULL;
}