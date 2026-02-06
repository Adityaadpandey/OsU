#include "vfs.h"

#include "fs.h"
#include "string.h"

static char name_buf[FS_MAX_NAME + 1];

static const char *normalize(const char *path) {
    if (!path || path[0] == '\0') {
        return 0;
    }

    while (*path == ' ') {
        path++;
    }
    if (*path == '/') {
        path++;
    }

    size_t i = 0;
    while (path[i] && path[i] != ' ' && i < FS_MAX_NAME) {
        if (path[i] == '/' || path[i] == '\\') {
            return 0;
        }
        if (path[i] == '.' && path[i + 1] == '.') {
            return 0;
        }
        name_buf[i] = path[i];
        i++;
    }
    name_buf[i] = '\0';

    if (i == 0 || path[i] != '\0') {
        return 0;
    }

    return name_buf;
}

void vfs_init(void) {
    fs_init();
}

int vfs_touch(const char *path) {
    const char *n = normalize(path);
    return n ? fs_touch(n) : -1;
}

int vfs_remove(const char *path) {
    const char *n = normalize(path);
    return n ? fs_remove(n) : -1;
}

int vfs_write(const char *path, const char *text) {
    const char *n = normalize(path);
    return n ? fs_write(n, text) : -1;
}

int vfs_append(const char *path, const char *text) {
    const char *n = normalize(path);
    return n ? fs_append(n, text) : -1;
}

int vfs_write_raw(const char *path, const char *data, size_t len) {
    const char *n = normalize(path);
    return n ? fs_write_raw(n, data, len) : -1;
}

const char *vfs_read_ptr(const char *path, size_t *len) {
    const char *n = normalize(path);
    return n ? fs_read_ptr(n, len) : 0;
}

int vfs_list_entry(size_t index, const char **name, size_t *len) {
    return fs_list_entry(index, name, len);
}
