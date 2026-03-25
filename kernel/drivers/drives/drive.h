#ifndef DRIVE_H
#define DRIVE_H
#include "stdint.h"
#include "sys/types.h"

#define KMAX_DRIVES 32
#define KDRIVE_NAME_MAX 128

struct kdrive_t;
typedef ssize_t (*kdrive_read)( struct kdrive_t *this, ssize_t start, ssize_t num_bytes, void *output );
typedef ssize_t (*kdrive_write)( struct kdrive_t *this, ssize_t start, ssize_t num_bytes, void *input );

typedef void(*kdmaster_init)();

struct kdrive_t
{
	char name[KDRIVE_NAME_MAX];
	uint32_t sector_size;
	uint32_t user_data1;
	uint32_t user_data2;

	kdrive_read read;
	kdrive_write write;

};

struct kdmaster_t
{
	kdmaster_init init;
};

void kdrive_register( struct kdrive_t drive );

#endif
