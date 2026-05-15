//Ember2819
#ifndef FAT16_H
#define FAT16_H

#include <stdint.h>
#include <partition/partition.h>
#include <drivers/drives.h>
#include <fs/fs.h>

struct drive_fs_t *fat16_drive_open( struct kdrive_t *drive, struct partition_t *partition );

int fat16_create_file( struct drive_fs_t *fs, char *name,
                       const uint8_t *content, size_t len );

int fat16_write_file( struct drive_fs_t *fs, char *name,
                      const uint8_t *content, size_t len );

int fat16_delete_file( struct drive_fs_t *fs, char *name );

int fat16_append_file( struct drive_fs_t *fs, char *name,
                       const uint8_t *content, size_t len );

int fat16_mkdir( struct drive_fs_t *fs, char *name );

void fat16_print_info( struct drive_fs_t *fs, uint8_t color );

#endif // FAT16_H
