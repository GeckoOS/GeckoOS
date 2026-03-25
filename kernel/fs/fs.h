#ifndef FS_H
#define FS_H

struct kfile_t
{

};

enum ferror_t
{
	FERR_SUCCESS,
	FERR_NOT_FOUND,
	FERR_IS_DIR,
};

extern ferror_t kerr;

#define KFLAG_READ 0x1
#define KFLAG_WRITE 0x2
#define KFLAG_APPEND 0x4
#define KFLAG_CREATE 0x8
#define KFLAG_TRUNC 0x10

struct kfile_t kopen( const char *path, int flags );

#endif
