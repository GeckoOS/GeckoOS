#include "drivers/vga.h"
#include "mem.h"
#include "terminal/terminal.h"
#include <fs/fat32.h>
#include <fs/fs.h>

struct drive_fs_t *fs;

struct drive_fs_t *fs_drive_open( struct kdrive_t *drive )
{
	struct partition_t part;

	part.type = FS_FAT32;
	part.lba = 0;
	part.size = SIZE_MAX;
	return fs_partition_open(drive, &part);
}

struct drive_fs_t *fs_partition_open( struct kdrive_t *drive, struct partition_t *partition )
{
	switch (partition->type)
	{
	case FS_FAT32:
		return fat32_drive_open(drive, partition);
	case FS_NONE:
	case FS_FAT12:
	case FS_FAT16:
		break;
	}
	return 0;
}

void fs_free_entries( struct fs_entries_t *entries )
{
	/* we do not have free lol, let that sink in */
}

Buffer_t readfile(unsigned char* fname) {
    struct fs_entries_t entries;
    int i, found;

    entries = fs->get_entries((void*)fs);
    found = -1;
    for (i = 0; i < (int)entries.count; i++) {
        if (entries.entries[i].type != ENTRY_FILE) continue;
        const char *a = entries.entries[i].file.name;
        const unsigned char *b = fname;
        int match = 1;
        while (*a && *b) {
            char ca = (*a >= 'a' && *a <= 'z') ? *a - 32 : *a;
            char cb = (*b >= 'a' && *b <= 'z') ? *b - 32 : *b;
            if (ca != cb) { match = 0; break; }
            a++; b++;
        }
        if (match && *a == '\0' && *b == '\0') { found = i; break; }
    }
    if (found == -1) {
        return (Buffer_t){NULL, 0};
    }

    uint8_t readbuf[128];
    int bytes, j = 0, o = 0;
	uint8_t* buffer = kmalloc(entries.entries[found].file.file_size);
    while ((bytes = entries.entries[found].file.read(
            (void*)&entries.entries[found].file, j * 128, 128, readbuf)) > 0) {
        j++;
        for (int k = 0; k < bytes; k++) {
            buffer[o] = readbuf[k];
            o++;
        }
    }
    return (Buffer_t){buffer, entries.entries[found].file.file_size};
}
int fsmount(int drive) {
    printc("\n", VGA_COLOR_WHITE);
    struct kdrive_t* d;
    if (!(d = get_kdrive(drive))) {
        // printc("No slave drive found. Is fat32.img attached as a second drive?\n", VGA_COLOR_RED);
        return 0;
    }
    fs = fs_drive_open(d);
    if (fs == 0) {
        printc("Filesystem mount failed. Is fat32.img a valid FAT32 image?\n", VGA_COLOR_RED);
        return -1;
    }
    printc("Filesystem mounted successfully.\n\n", VGA_COLOR_WHITE);
    return 1;
}