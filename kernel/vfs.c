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
        /* Allow path separators for chdir */
        if (path[i] == '.' && path[i + 1] == '.' && (path[i + 2] == '\0' || path[i + 2] == '/')) {
            /* Allow .. for parent directory */
            name_buf[i] = path[i];
            i++;
            continue;
        }
        if (path[i] == '/' || path[i] == '\\') {
            return 0; /* Don't allow subdirectory paths in file operations */
        }
        name_buf[i] = path[i];
        i++;
    }
    name_buf[i] = '\0';

    if (i == 0) {
        return 0;
    }

    return name_buf;
}

/* Normalize for directory operations - allows .. */
static const char *normalize_dir(const char *path) {
    if (!path || path[0] == '\0') {
        return 0;
    }

    while (*path == ' ') {
        path++;
    }

    /* Handle absolute paths */
    if (*path == '/') {
        return path; /* Return as-is for chdir to handle */
    }

    size_t i = 0;
    while (path[i] && path[i] != ' ' && i < FS_MAX_NAME) {
        name_buf[i] = path[i];
        i++;
    }
    name_buf[i] = '\0';

    if (i == 0) {
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

/* Directory operations */

int vfs_mkdir(const char *path) {
    const char *n = normalize(path);
    return n ? fs_mkdir(n) : -1;
}

int vfs_rmdir(const char *path) {
    const char *n = normalize(path);
    return n ? fs_rmdir(n) : -1;
}

int vfs_chdir(const char *path) {
    const char *n = normalize_dir(path);
    return n ? fs_chdir(n) : -1;
}

const char *vfs_getcwd(void) {
    return fs_getcwd();
}

int vfs_is_dir(const char *path) {
    const char *n = normalize(path);
    return n ? fs_is_dir(n) : 0;
}

int vfs_list_dir_entry(size_t index, const char **name, size_t *len, int *is_dir) {
    return fs_list_dir_entry(index, name, len, is_dir);
}
