#ifndef DRIVE_FS_H
#define DRIVE_FS_H

#include "../mem.h"
#include "../partition/partition.h"
#include "../drivers/drives.h"
#include "stdint.h"
#define FILENAME_MAX 256

typedef size_t (*fn_df_read)(struct drive_file_t *, size_t offset, size_t count, uint8_t *data);
typedef size_t (*fn_df_write)(struct drive_file_t *, size_t offset, size_t count, const uint8_t *data);
typedef size_t (*fn_df_truncate)(struct drive_file_t *, size_t size, const uint8_t *data);
typedef size_t (*fn_df_open)(struct drive_file_t *);
typedef size_t (*fn_df_close)(struct drive_file_t *);

typedef size_t (*fn_get_entry_count)(struct drive_dir_t *);
typedef union drive_entry_t *(*fn_get_entries)(struct drive_dir_t *);

typedef union drive_entry_t *(*fn_get_root_entry)(struct drive_fs_t *);

enum drive_entry_type_t
{
	ENTRY_FILE,
	ENTRY_DIRECTORY,
	ENTRY_LINK,
};

struct drive_file_t
{
	enum drive_entry_type_t type;
	char name[FILENAME_MAX];
	size_t userdata1;
	size_t userdata2;
	size_t userdata3;
	size_t userdata4;
	fn_df_open open;
	fn_df_close close;
	fn_df_read read;
	fn_df_write write;
	fn_df_truncate truncate;
};

struct drive_dir_t
{
	enum drive_entry_type_t type;
	char name[FILENAME_MAX];
	size_t userdata1;
	size_t userdata2;
	size_t userdata3;
	size_t userdata4;
	fn_get_entry_count get_entry_count;
	fn_get_entries get_entries;
};

union drive_entry_t
{
	struct drive_dir_t dir;
	struct drive_file_t file;
};

struct drive_fs_t
{
	fn_get_root_entry get_root_entry;
};

struct drive_fs_t *drive_open( struct kdrive_t *drive );
struct drive_fs_t *partition_open( struct partition_t *partition );

#endif
