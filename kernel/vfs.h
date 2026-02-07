#ifndef VFS_H
#define VFS_H

#include <stddef.h>

void vfs_init(void);
int vfs_touch(const char *path);
int vfs_remove(const char *path);
int vfs_write(const char *path, const char *text);
int vfs_append(const char *path, const char *text);
int vfs_write_raw(const char *path, const char *data, size_t len);
const char *vfs_read_ptr(const char *path, size_t *len);
int vfs_list_entry(size_t index, const char **name, size_t *len);

/* Directory operations */
int vfs_mkdir(const char *path);
int vfs_rmdir(const char *path);
int vfs_chdir(const char *path);
const char *vfs_getcwd(void);
int vfs_is_dir(const char *path);
int vfs_list_dir_entry(size_t index, const char **name, size_t *len, int *is_dir);

#endif
