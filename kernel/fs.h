#ifndef FS_H
#define FS_H

#include <stddef.h>

#define FS_MAX_FILES 32
#define FS_MAX_NAME 23
#define FS_MAX_FILE_SIZE 4096

void fs_init(void);
int fs_touch(const char *name);
int fs_remove(const char *name);
int fs_write(const char *name, const char *text);
int fs_append(const char *name, const char *text);
int fs_write_raw(const char *name, const char *data, size_t len);
const char *fs_read_ptr(const char *name, size_t *len);
int fs_list_entry(size_t index, const char **name, size_t *len);

#endif
