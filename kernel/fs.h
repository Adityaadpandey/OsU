#ifndef FS_H
#define FS_H

#include <stddef.h>

#define FS_MAX_FILES 32
#define FS_MAX_NAME 23
#define FS_MAX_FILE_SIZE 4096
#define FS_MAX_PATH 128

#define FS_ATTR_DIRECTORY 0x10
#define FS_ATTR_ARCHIVE   0x20

void fs_init(void);
int fs_touch(const char *name);
int fs_remove(const char *name);
int fs_write(const char *name, const char *text);
int fs_append(const char *name, const char *text);
int fs_write_raw(const char *name, const char *data, size_t len);
const char *fs_read_ptr(const char *name, size_t *len);
int fs_list_entry(size_t index, const char **name, size_t *len);

/* Directory operations */
int fs_mkdir(const char *name);
int fs_rmdir(const char *name);
int fs_chdir(const char *path);
const char *fs_getcwd(void);
int fs_is_dir(const char *name);
int fs_list_dir_entry(size_t index, const char **name, size_t *len, int *is_dir);

#endif
