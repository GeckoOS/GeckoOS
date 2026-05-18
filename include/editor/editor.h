//ember2819
#ifndef EDITOR_H
#define EDITOR_H

#include <stdint.h>
#include <fs/fs.h>

// Maximum file size the editor can handle
#define EDITOR_MAX_FILESIZE 4096
// Maximum number of lines
#define EDITOR_MAX_LINES    128
// Maximum line length
#define EDITOR_MAX_LINELEN  79

void editor_run(struct drive_fs_t *fs, const char *filename, uint8_t color);

#endif // EDITOR_H
