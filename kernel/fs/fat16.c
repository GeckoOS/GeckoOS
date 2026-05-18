#include <fs/fat16.h>
#include <drivers/ata.h>
#include <mem.h>
#include <terminal/terminal.h>
#include <colors.h>
#include <partition/partition.h>
#include <fs/fs.h>
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
    struct kdrive_t *drive;
} FAT16_Volume;

typedef struct {
    uint32_t  file_size;         // Total file size in bytes
    uint32_t  position;
    uint16_t  first_cluster;     // Starting cluster of the file
    uint16_t  current_cluster;   // Cluster currently being read
    uint32_t  cluster_offset;    // Byte offset within the current cluster
    uint8_t   valid;
} FAT16_File;

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

static uint8_t sector_buf[ATA_SECTOR_SIZE];

static uint32_t cluster_to_lba(FAT16_Volume *pvol, uint16_t cluster) {
    return pvol->data_lba + ((uint32_t)(cluster - 2) * pvol->bpb.sectors_per_cluster);
}

static uint16_t fat16_next_cluster(struct kdrive_t *drive, FAT16_Volume *pvol, uint16_t cluster) {
    // Each FAT16 entry is 2 bytes. Calculate which sector of the FAT holds it.
    uint32_t fat_offset  = cluster * 2;
    uint32_t fat_sector  = pvol->fat_lba + (fat_offset / drive->sector_size);
    uint32_t byte_offset = fat_offset % drive->sector_size;

    if (drive->read((void*)drive, fat_sector, 1, sector_buf) < 0) return 0xFFFF;

    uint16_t entry = *(uint16_t *)(sector_buf + byte_offset);
    return entry;
}

static size_t fat16_file_read( struct drive_file_t *file, size_t offset, size_t len, uint8_t *buf )
{
	size_t remaining;
	size_t bytes_read;

	uint16_t current_cluster;
	size_t cluster_size;
	size_t cluster_offset;

	uint16_t clusters_to_skip;
	FAT16_Volume *pvolume;
	
	size_t lba;
	size_t i;

	if ( offset >= file->file_size )
		return 0;
	remaining = file->file_size-offset;
	if ( len > remaining ) len = remaining;


	bytes_read = file->file_size;
	current_cluster = file->userdata2;
	pvolume = file->fs->userdata1;
	cluster_size = pvolume->bpb.bytes_per_sector * pvolume->bpb.sectors_per_cluster;
	clusters_to_skip = offset/cluster_size;
	cluster_offset = offset%cluster_size;

	for ( i = 0; i < clusters_to_skip; i++ )
	{
		current_cluster = fat16_next_cluster(file->drive, pvolume, current_cluster);

		if (current_cluster >= FAT16_CLUSTER_BAD)
			return -1;
	}
	bytes_read = 0;

	while(bytes_read < len)
	{
		if (current_cluster >= FAT16_CLUSTER_BAD)
			break;

		int32_t bytes_in_cluster = cluster_size - cluster_offset;

		uint32_t to_copy = len - bytes_read;
		if (to_copy > bytes_in_cluster) to_copy = bytes_in_cluster;

		// Read the cluster sector by sector
		uint32_t lba = cluster_to_lba(pvolume, current_cluster);
		uint32_t offset_in_cluster = cluster_offset;

		// We'll read sector by sector within the cluster
		while (to_copy > 0) {
			uint32_t sector_index   = offset_in_cluster / file->drive->sector_size;
			uint32_t offset_in_sec  = offset_in_cluster % file->drive->sector_size;
			uint32_t avail_in_sec   = file->drive->sector_size - offset_in_sec;
			uint32_t chunk          = to_copy < avail_in_sec ? to_copy : avail_in_sec;

			if (file->drive->read((void*)file->drive, lba + sector_index, 1, sector_buf) < 0)
				return bytes_read > 0 ? (int)bytes_read : -1;

			memcpy(buf + bytes_read, sector_buf + offset_in_sec, chunk);

			bytes_read        += chunk;
			offset_in_cluster += chunk;
			to_copy           -= chunk;
		}

		// Recalculate cleanly
		cluster_offset = offset_in_cluster;

		if (cluster_offset >= cluster_size) {
			cluster_offset = 0;
			current_cluster = fat16_next_cluster(file->drive, pvolume,current_cluster);
		}
	}
	return bytes_read;
}

static int fat16_write_fat(struct kdrive_t *drive, FAT16_Volume *pvol,
                           uint16_t cluster, uint16_t value)
{
	uint32_t fat_offset  = cluster * 2;
	uint32_t fat_sector  = pvol->fat_lba + (fat_offset / drive->sector_size);
	uint32_t byte_offset = fat_offset % drive->sector_size;
	uint8_t  tmp[ATA_SECTOR_SIZE];

	if (drive->read((void*)drive, fat_sector, 1, tmp) < 0) return -1;
	*(uint16_t *)(tmp + byte_offset) = value;
	if (drive->write((void*)drive, fat_sector, 1, tmp) < 0) return -1;
	if (pvol->bpb.num_fats > 1) {
		uint32_t fat2_sector = fat_sector + pvol->bpb.sectors_per_fat;
		if (drive->write((void*)drive, fat2_sector, 1, tmp) < 0) return -1;
	}
	return 0;
}

static uint16_t fat16_alloc_cluster(struct kdrive_t *drive, FAT16_Volume *pvol)
{
	uint32_t fat_sectors = pvol->bpb.sectors_per_fat;
	uint32_t entries_per_sector = drive->sector_size / 2;
	uint8_t  tmp[ATA_SECTOR_SIZE];
	uint32_t s, e;

	for (s = 0; s < fat_sectors; s++) {
		if (drive->read((void*)drive, pvol->fat_lba + s, 1, tmp) < 0) return 0;
		for (e = 0; e < entries_per_sector; e++) {
			uint16_t val = *(uint16_t *)(tmp + e * 2);
			uint16_t cluster = (uint16_t)(s * entries_per_sector + e);
			if (cluster < 2) continue; 
			if (val == FAT16_CLUSTER_FREE) {
				*(uint16_t *)(tmp + e * 2) = FAT16_CLUSTER_EOC;
				if (drive->write((void*)drive, pvol->fat_lba + s, 1, tmp) < 0) return 0;
				if (pvol->bpb.num_fats > 1)
					drive->write((void*)drive, pvol->fat_lba + pvol->bpb.sectors_per_fat + s, 1, tmp);
				return cluster;
			}
		}
	}
	return 0; // disk full
}

static size_t fat16_file_write(struct drive_file_t *file, size_t offset,
                               size_t len, const uint8_t *buf)
{
	FAT16_Volume *pvolume = (FAT16_Volume *)file->fs->userdata1;
	size_t cluster_size   = pvolume->bpb.bytes_per_sector * pvolume->bpb.sectors_per_cluster;
	uint16_t current_cluster = (uint16_t)file->userdata2;
	size_t bytes_written  = 0;
	size_t cluster_offset;
	uint8_t  tmp[ATA_SECTOR_SIZE];

	if (offset >= file->file_size) return 0;
	if (offset + len > file->file_size) len = file->file_size - offset;

	uint16_t clusters_to_skip = (uint16_t)(offset / cluster_size);
	cluster_offset = offset % cluster_size;
	for (uint16_t i = 0; i < clusters_to_skip; i++) {
		current_cluster = fat16_next_cluster(file->drive, pvolume, current_cluster);
		if (current_cluster >= FAT16_CLUSTER_BAD) return bytes_written;
	}

	while (bytes_written < len) {
		if (current_cluster >= FAT16_CLUSTER_BAD) break;

		uint32_t lba = cluster_to_lba(pvolume, current_cluster);
		size_t offset_in_cluster = cluster_offset;

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
			current_cluster = fat16_next_cluster(file->drive, pvolume, current_cluster);
	}
	return bytes_written;
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

int fat16_create_file(struct drive_fs_t *fs, char *name,
                      const uint8_t *content, size_t len)
{
	FAT16_Volume *pvol = (FAT16_Volume *)fs->userdata1;
	struct kdrive_t *drive = fs->drive;
	uint32_t root_dir_sectors = ((uint32_t)pvol->bpb.root_entry_count * 32
		+ pvol->bpb.bytes_per_sector - 1) / pvol->bpb.bytes_per_sector;

	uint8_t *root_buf = kmalloc(root_dir_sectors * drive->sector_size);
	if (drive->read((void*)drive, pvol->root_dir_lba, root_dir_sectors, root_buf) < 0)
		return -1;

	FAT16_DirEntry *slot = 0;
	uint32_t total_entries = root_dir_sectors * (drive->sector_size / 32);
	for (uint32_t i = 0; i < total_entries; i++) {
		FAT16_DirEntry *e = (FAT16_DirEntry *)root_buf + i;
		if (e->name[0] == FAT16_ENTRY_FREE || e->name[0] == FAT16_ENTRY_END) {
			slot = e;
			break;
		}
	}
	if (!slot) return -1;

	uint16_t first_cluster = 0;
	uint16_t prev_cluster  = 0;
	size_t cluster_size = pvol->bpb.bytes_per_sector * pvol->bpb.sectors_per_cluster;
	size_t remaining    = len;

	size_t clusters_needed = (len == 0) ? 1 : (len + cluster_size - 1) / cluster_size;

	for (size_t c = 0; c < clusters_needed; c++) {
		uint16_t cl = fat16_alloc_cluster(drive, pvol);
		if (cl == 0) return -1;

		if (first_cluster == 0) first_cluster = cl;
		if (prev_cluster  != 0) fat16_write_fat(drive, pvol, prev_cluster, cl);
		prev_cluster = cl;

		uint32_t lba = cluster_to_lba(pvol, cl);
		uint8_t  tmp[ATA_SECTOR_SIZE];
		size_t   written_in_cluster = 0;

		for (uint32_t s = 0; s < pvol->bpb.sectors_per_cluster; s++) {
			memset(tmp, 0, drive->sector_size);
			size_t chunk = (remaining > drive->sector_size) ? drive->sector_size : remaining;
			if (chunk > 0) {
				size_t src_off = len - remaining;
				memcpy(tmp, content + src_off, chunk);
				remaining -= chunk;
			}
			drive->write((void*)drive, lba + s, 1, tmp);
		}
	}

	memset(slot, 0, sizeof(FAT16_DirEntry));
	encode_83_name(name, slot->name, slot->ext);
	slot->attributes   = FAT16_ATTR_ARCHIVE;
	slot->first_cluster = first_cluster;
	slot->file_size    = (uint32_t)len;
	slot->write_date   = (uint16_t)((44 << 9) | (1 << 5) | 1);
	slot->write_time   = 0;
	slot->create_date  = slot->write_date;
	slot->create_time  = 0;

	if (drive->write((void*)drive, pvol->root_dir_lba, root_dir_sectors, root_buf) < 0)
		return -1;

	return 0;
}

int fat16_write_file(struct drive_fs_t *fs, char *name,
                     const uint8_t *content, size_t len)
{
    FAT16_Volume   *pvol  = (FAT16_Volume *)fs->userdata1;
    struct kdrive_t *drive = fs->drive;

    uint32_t root_dir_sectors = ((uint32_t)pvol->bpb.root_entry_count * 32
        + pvol->bpb.bytes_per_sector - 1) / pvol->bpb.bytes_per_sector;

    // Load entire root directory into a buffer
    uint8_t *root_buf = kmalloc(root_dir_sectors * drive->sector_size);
    if (drive->read((void*)drive, pvol->root_dir_lba, root_dir_sectors, root_buf) < 0)
        return -1;

    // Encode the target name into 8.3 format for comparison
    uint8_t target8[8], target3[3];
    encode_83_name(name, target8, target3);

    // Find the matching directory entry
    FAT16_DirEntry *slot = 0;
    uint32_t total_entries = root_dir_sectors * (drive->sector_size / 32);
    for (uint32_t i = 0; i < total_entries; i++) {
        FAT16_DirEntry *e = (FAT16_DirEntry *)root_buf + i;
        if (e->name[0] == FAT16_ENTRY_END)  break;
        if (e->name[0] == FAT16_ENTRY_FREE) continue;
        if (e->attributes & FAT16_ATTR_VOLUME_ID) continue;
        if ((e->attributes & FAT16_ATTR_LFN) == FAT16_ATTR_LFN) continue;
        if (e->attributes & FAT16_ATTR_DIRECTORY) continue;

        // Compare 8.3 name bytes
        int match = 1;
        for (int k = 0; k < 8; k++) if (e->name[k] != target8[k]) { match = 0; break; }
        if (match) for (int k = 0; k < 3; k++) if (e->ext[k] != target3[k]) { match = 0; break; }
        if (match) { slot = e; break; }
    }

    // File not found – fall back to create
    if (!slot) {
        return fat16_create_file(fs, name, content, len);
    }

    // Free the old cluster chain
    uint16_t cluster = slot->first_cluster;
    while (cluster >= 2 && cluster < FAT16_CLUSTER_BAD) {
        uint16_t next = fat16_next_cluster(drive, pvol, cluster);
        fat16_write_fat(drive, pvol, cluster, FAT16_CLUSTER_FREE);
        cluster = next;
    }

    // Allocate a new chain and write content
    size_t cluster_size    = pvol->bpb.bytes_per_sector * pvol->bpb.sectors_per_cluster;
    size_t clusters_needed = (len == 0) ? 1 : (len + cluster_size - 1) / cluster_size;

    uint16_t first_cluster = 0;
    uint16_t prev_cluster  = 0;
    size_t   remaining     = len;

    for (size_t c = 0; c < clusters_needed; c++) {
        uint16_t cl = fat16_alloc_cluster(drive, pvol);
        if (cl == 0) return -1;   // disk full

        if (first_cluster == 0) first_cluster = cl;
        if (prev_cluster  != 0) fat16_write_fat(drive, pvol, prev_cluster, cl);
        prev_cluster = cl;

        uint32_t lba = cluster_to_lba(pvol, cl);
        uint8_t  tmp[ATA_SECTOR_SIZE];

        for (uint32_t s = 0; s < pvol->bpb.sectors_per_cluster; s++) {
            memset(tmp, 0, drive->sector_size);
            size_t chunk = (remaining > drive->sector_size) ? drive->sector_size : remaining;
            if (chunk > 0) {
                size_t src_off = len - remaining;
                memcpy(tmp, content + src_off, chunk);
                remaining -= chunk;
            }
            drive->write((void*)drive, lba + s, 1, tmp);
        }
    }

    // Update directory entry with new chain start and size
    slot->first_cluster = first_cluster;
    slot->file_size     = (uint32_t)len;
    slot->write_date    = (uint16_t)((44 << 9) | (1 << 5) | 1);
    slot->write_time    = 0;

    // Flush updated root directory back to disk
    if (drive->write((void*)drive, pvol->root_dir_lba, root_dir_sectors, root_buf) < 0)
        return -1;

    return 0;
}

void fat16_print_info(struct drive_fs_t *fs, uint8_t color)
{
	FAT16_Volume *v = (FAT16_Volume *)fs->userdata1;
	printc("-- FAT16 Volume Info --\n", color);
	printc("  Bytes/sector:      ", color); print_int(v->bpb.bytes_per_sector);    printc("\n", color);
	printc("  Sectors/cluster:   ", color); print_int(v->bpb.sectors_per_cluster); printc("\n", color);
	printc("  Reserved sectors:  ", color); print_int(v->bpb.reserved_sectors);    printc("\n", color);
	printc("  FATs:              ", color); print_int(v->bpb.num_fats);             printc("\n", color);
	printc("  Root entries:      ", color); print_int(v->bpb.root_entry_count);    printc("\n", color);
	printc("  Sectors/FAT:       ", color); print_int(v->bpb.sectors_per_fat);     printc("\n", color);
	printc("  Total sectors:     ", color); print_int(v->total_sectors);           printc("\n", color);
	printc("  FAT LBA:           ", color); print_int(v->fat_lba);                 printc("\n", color);
	printc("  Root dir LBA:      ", color); print_int(v->root_dir_lba);            printc("\n", color);
	printc("  Data area LBA:     ", color); print_int(v->data_lba);                printc("\n", color);
	printc("-----------------------\n", color);
}

static struct fs_entries_t fat16_get_root_entries( struct drive_fs_t *fs )
{
	FAT16_Volume *pvolume;
	FAT16_DirEntry *entries;
	uint32_t root_dir_sectors;
	uint32_t sector;
	ssize_t n;
	ssize_t i;
	uint8_t first;
	struct fs_entries_t fs_entries;
	uint8_t *sector_buf;
	drive_entry_t *entry;

	pvolume = (FAT16_Volume*)fs->userdata1;
	root_dir_sectors = ((uint32_t)pvolume->bpb.root_entry_count * 32
			+ pvolume->bpb.bytes_per_sector - 1)
		/ pvolume->bpb.bytes_per_sector;
	sector_buf = kmalloc(root_dir_sectors*fs->drive->sector_size);
	fs_entries.count = 0;
	fs_entries.entries = 0;

	/* we do not want to estimate size */
	n = fs->drive->read((void*)fs->drive, pvolume->root_dir_lba, root_dir_sectors, sector_buf);
	if (n < 0)
		return fs_entries;

	for ( sector = 0; sector < root_dir_sectors; sector++ )
	{
		if (n < 0)
			break;
		entries = (FAT16_DirEntry*)(sector_buf + sector * pvolume->bpb.bytes_per_sector);

		for ( i = 0; i < pvolume->bpb.bytes_per_sector / 32; i++ )
		{
			first = entries[i].name[0]; 

			if (first == FAT16_ENTRY_END) goto done_listing_count;
			if (first == FAT16_ENTRY_FREE) continue;
			if (entries[i].attributes & FAT16_ATTR_VOLUME_ID) continue;
			if ((entries[i].attributes & FAT16_ATTR_LFN) == FAT16_ATTR_LFN) continue;

			fs_entries.count++;
		}
	}
done_listing_count:
	if (fs_entries.count == 0)
		return fs_entries;

	fs_entries.entries = kmalloc(fs_entries.count*sizeof(drive_entry_t));
	memset(fs_entries.entries, 0, fs_entries.count*sizeof(drive_entry_t));

	n = 0; /* global entry index across all sectors */
	for ( sector = 0; sector < root_dir_sectors; sector++ )
	{
		entries = (FAT16_DirEntry*)(sector_buf + sector * pvolume->bpb.bytes_per_sector);

		for ( i = 0; i < pvolume->bpb.bytes_per_sector / 32; i++ )
		{
			first = entries[i].name[0]; 

			if (first == FAT16_ENTRY_END) goto done_listing;
			if (first == FAT16_ENTRY_FREE) continue;
			if (entries[i].attributes & FAT16_ATTR_VOLUME_ID) continue;
			if ((entries[i].attributes & FAT16_ATTR_LFN) == FAT16_ATTR_LFN) continue;

			entry = &fs_entries.entries[n];

			if (entries[i].attributes & FAT16_ATTR_DIRECTORY) {
				fs_entries.entries[n].type = ENTRY_DIRECTORY;
				entry->dir.fs = fs;
				entry->dir.drive = fs->drive;
				format_83_name(entries[i].name, entries[i].ext, fs_entries.entries[n].dir.name);
			}
			else {
				entry->type = ENTRY_FILE;
				entry->file.fs = fs;
				entry->file.drive = fs->drive;
				entry->file.file_size = entries[i].file_size;
				entry->file.userdata1 = pvolume;
				entry->file.userdata2 = entries[i].first_cluster;
				entry->file.read  = (fn_df_read)fat16_file_read;
				entry->file.write = (fn_df_write)fat16_file_write;
				format_83_name(entries[i].name, entries[i].ext, fs_entries.entries[n].file.name);
			}
			n++;
		}
	}
	/* todo: free memory */


done_listing:
	return fs_entries;
}

struct drive_fs_t *fat16_drive_open( struct kdrive_t *drive, struct partition_t *partition )
{
	struct drive_fs_t *filesystem;
	FAT16_Volume volume;
	FAT16_Volume *pvolume;
	uint32_t root_dir_sectors;
	uint32_t sec;

	memset(&volume, 0, sizeof(FAT16_Volume));
	volume.partition_lba = partition->lba;
	volume.drive = drive;
	if (drive->read((void*)drive, partition->lba, 1, sector_buf) < 0)
		return NULL;

	memcpy(&volume.bpb, sector_buf, sizeof(FAT16_BPB));

	if (volume.bpb.bytes_per_sector == 0 || volume.bpb.sectors_per_cluster == 0)
	{
		kprintf(SEVERITY_ERROR, "FAT16] Error: invalid BPB (zero bytes/sector or sectors/cluster)\n");
		return NULL;
	}

	volume.fat_lba = partition->lba + volume.bpb.reserved_sectors;
	volume.root_dir_lba = volume.fat_lba + ((uint32_t)volume.bpb.num_fats*volume.bpb.sectors_per_fat);

	root_dir_sectors = ((uint32_t)volume.bpb.root_entry_count * 32
                                  + volume.bpb.bytes_per_sector - 1)
                                 / volume.bpb.bytes_per_sector;

	volume.data_lba = volume.root_dir_lba + root_dir_sectors;

	volume.total_sectors = volume.bpb.total_sectors_16 != 0
		? volume.bpb.total_sectors_16
		: volume.bpb.total_sectors_32;
	volume.mounted = 1;

	filesystem = kmalloc(sizeof(struct drive_fs_t));
	pvolume = kmalloc(sizeof(FAT16_Volume));
	memcpy(pvolume, &volume, sizeof(FAT16_Volume));
	filesystem->drive = drive;
	filesystem->userdata1 = pvolume;
	filesystem->get_entries = (fn_root_get_entries)fat16_get_root_entries;
	return filesystem;
};

struct drive_fs_t *fat16_drive_close( struct drive_fs_t *fs )
{
	return NULL;
}

int fat16_delete_file(struct drive_fs_t *fs, char *name) {
    if (!fs) return -1;
    FAT16_Volume   *pvol  = (FAT16_Volume *)fs->userdata1;
    struct kdrive_t *drive = fs->drive;

    uint32_t root_dir_sectors = ((uint32_t)pvol->bpb.root_entry_count * 32
        + pvol->bpb.bytes_per_sector - 1) / pvol->bpb.bytes_per_sector;

    uint8_t *root_buf = kmalloc(root_dir_sectors * drive->sector_size);
    if (drive->read((void*)drive, pvol->root_dir_lba, root_dir_sectors, root_buf) < 0)
        return -1;

    // Build target 8.3 name for comparison
    uint8_t target8[8], target3[3];
    encode_83_name(name, target8, target3);

    uint32_t total_entries = root_dir_sectors * (drive->sector_size / 32);
    for (uint32_t i = 0; i < total_entries; i++) {
        FAT16_DirEntry *e = (FAT16_DirEntry *)root_buf + i;
        if (e->name[0] == FAT16_ENTRY_END)  break;   // no more entries
        if (e->name[0] == FAT16_ENTRY_FREE) continue; // already deleted
        if (e->attributes & FAT16_ATTR_VOLUME_ID) continue;

        int match = 1;
        for (int k = 0; k < 8; k++) if (e->name[k] != target8[k]) { match = 0; break; }
        if (match) for (int k = 0; k < 3; k++) if (e->ext[k] != target3[k]) { match = 0; break; }

        if (match) {
            e->name[0] = FAT16_ENTRY_FREE;
            drive->write((void*)drive, pvol->root_dir_lba, root_dir_sectors, root_buf);
            return 0;
        }
    }
    return -1;
}

//ember2819
int fat16_append_file(struct drive_fs_t *fs, char *name,
                      const uint8_t *content, size_t len) {
    if (!fs) return -1;

    // Try to read existing content first
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
        int chunk, j = 0;
        while (total < 4000) {
            uint8_t tmp[128];
            chunk = entries.entries[found].file.read(
                (void*)&entries.entries[found].file, j * 128, 128, tmp);
            if (chunk <= 0) break;
            for (int k = 0; k < chunk && total < 4000; k++)
                appbuf[total++] = tmp[k];
            j++;
        }
        fat16_delete_file(fs, name);
    }

    // Append new content
    for (size_t i = 0; i < len && total < 4095; i++)
        appbuf[total++] = content[i];

    return fat16_create_file(fs, name, appbuf, (size_t)total);
}

//ember2819
int fat16_mkdir(struct drive_fs_t *fs, char *name) {
    if (!fs) return -1;
    FAT16_Volume   *pvol  = (FAT16_Volume *)fs->userdata1;
    struct kdrive_t *drive = fs->drive;

    uint32_t root_dir_sectors = ((uint32_t)pvol->bpb.root_entry_count * 32
        + pvol->bpb.bytes_per_sector - 1) / pvol->bpb.bytes_per_sector;

    uint8_t *root_buf = kmalloc(root_dir_sectors * drive->sector_size);
    if (drive->read((void*)drive, pvol->root_dir_lba, root_dir_sectors, root_buf) < 0)
        return -1;

    uint32_t total_entries = root_dir_sectors * (drive->sector_size / 32);
    for (uint32_t i = 0; i < total_entries; i++) {
        FAT16_DirEntry *e = (FAT16_DirEntry *)root_buf + i;
        if (e->name[0] == FAT16_ENTRY_FREE || e->name[0] == FAT16_ENTRY_END) {
            // Found a free slot – build a minimal directory entry
            memset(e, 0, sizeof(FAT16_DirEntry));
            encode_83_name(name, e->name, e->ext);
            e->attributes   = FAT16_ATTR_DIRECTORY;
            e->first_cluster = 0;  // no cluster allocated yet
            e->file_size     = 0;
            e->write_date    = (uint16_t)((44 << 9) | (1 << 5) | 1);
            drive->write((void*)drive, pvol->root_dir_lba, root_dir_sectors, root_buf);
            return 0;
        }
    }
    return -1;  // root directory full
}
