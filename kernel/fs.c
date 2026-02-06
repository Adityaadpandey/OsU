#include "fs.h"

#include <stdint.h>

#include "disk.h"
#include "string.h"

#define FAT_LBA_START 4096u
#define FAT_TOTAL_SECTORS 8192u
#define FAT_SECTORS_PER_CLUSTER 1u
#define FAT_RESERVED_SECTORS 1u
#define FAT_FAT_COUNT 1u
#define FAT_ROOT_ENTRIES 128u
#define FAT_SECTORS_PER_FAT 32u

#define FAT_ROOT_DIR_SECTORS ((FAT_ROOT_ENTRIES * 32u + 511u) / 512u)
#define FAT_FIRST_DATA_SECTOR (FAT_RESERVED_SECTORS + (FAT_FAT_COUNT * FAT_SECTORS_PER_FAT) + FAT_ROOT_DIR_SECTORS)
#define FAT_DATA_SECTORS (FAT_TOTAL_SECTORS - FAT_FIRST_DATA_SECTOR)
#define FAT_CLUSTER_COUNT (FAT_DATA_SECTORS / FAT_SECTORS_PER_CLUSTER)
#define FAT_CLUSTER_MIN 2u
#define FAT_CLUSTER_MAX (FAT_CLUSTER_MIN + FAT_CLUSTER_COUNT - 1u)

#define FAT_ATTR_ARCHIVE 0x20
#define FAT16_EOC 0xFFFFu

#define FAT_EU_SIZE (FAT_SECTORS_PER_FAT * 512u)
#define ROOT_EU_SIZE (FAT_ROOT_DIR_SECTORS * 512u)

typedef struct {
    uint8_t jmp_boot[3];
    char oem_name[8];
    uint16_t bytes_per_sector;
    uint8_t sectors_per_cluster;
    uint16_t reserved_sector_count;
    uint8_t num_fats;
    uint16_t root_entry_count;
    uint16_t total_sectors_16;
    uint8_t media;
    uint16_t fat_size_16;
    uint16_t sectors_per_track;
    uint16_t num_heads;
    uint32_t hidden_sectors;
    uint32_t total_sectors_32;
    uint8_t drive_number;
    uint8_t reserved1;
    uint8_t boot_signature;
    uint32_t volume_id;
    char volume_label[11];
    char fs_type[8];
    uint8_t boot_code[448];
    uint16_t signature;
} __attribute__((packed)) fat16_boot_sector_t;

typedef struct {
    char name[8];
    char ext[3];
    uint8_t attr;
    uint8_t ntres;
    uint8_t crt_time_tenth;
    uint16_t crt_time;
    uint16_t crt_date;
    uint16_t lst_acc_date;
    uint16_t fst_clus_hi;
    uint16_t wrt_time;
    uint16_t wrt_date;
    uint16_t fst_clus_lo;
    uint32_t file_size;
} __attribute__((packed)) fat_dir_entry_t;

static uint8_t fat_buf[FAT_EU_SIZE];
static uint8_t root_buf[ROOT_EU_SIZE];
static char read_cache[FS_MAX_FILE_SIZE + 1];
static int fs_ready;

static uint32_t root_lba(void) {
    return FAT_LBA_START + FAT_RESERVED_SECTORS + (FAT_FAT_COUNT * FAT_SECTORS_PER_FAT);
}

static uint32_t data_lba(void) {
    return FAT_LBA_START + FAT_FIRST_DATA_SECTOR;
}

static uint32_t cluster_lba(uint16_t cluster) {
    return data_lba() + ((uint32_t)(cluster - FAT_CLUSTER_MIN) * FAT_SECTORS_PER_CLUSTER);
}

static int write_boot_sector(void) {
    fat16_boot_sector_t bs;
    memset(&bs, 0, sizeof(bs));

    bs.jmp_boot[0] = 0xEB;
    bs.jmp_boot[1] = 0x3C;
    bs.jmp_boot[2] = 0x90;
    memcpy(bs.oem_name, "MINIOS  ", 8);
    bs.bytes_per_sector = 512;
    bs.sectors_per_cluster = FAT_SECTORS_PER_CLUSTER;
    bs.reserved_sector_count = FAT_RESERVED_SECTORS;
    bs.num_fats = FAT_FAT_COUNT;
    bs.root_entry_count = FAT_ROOT_ENTRIES;
    bs.total_sectors_16 = FAT_TOTAL_SECTORS;
    bs.media = 0xF8;
    bs.fat_size_16 = FAT_SECTORS_PER_FAT;
    bs.sectors_per_track = 63;
    bs.num_heads = 16;
    bs.hidden_sectors = FAT_LBA_START;
    bs.total_sectors_32 = 0;
    bs.drive_number = 0x80;
    bs.boot_signature = 0x29;
    bs.volume_id = 0x20260206;
    memcpy(bs.volume_label, "OSUVOLUME  ", 11);
    memcpy(bs.fs_type, "FAT16   ", 8);
    bs.signature = 0xAA55;

    return disk_write_sectors(FAT_LBA_START, 1, &bs);
}

static uint16_t fat_get(uint16_t cluster) {
    uint32_t off = (uint32_t)cluster * 2u;
    return (uint16_t)(fat_buf[off] | ((uint16_t)fat_buf[off + 1] << 8));
}

static void fat_set(uint16_t cluster, uint16_t val) {
    uint32_t off = (uint32_t)cluster * 2u;
    fat_buf[off] = (uint8_t)(val & 0xFF);
    fat_buf[off + 1] = (uint8_t)((val >> 8) & 0xFF);
}

static int flush_fat(void) {
    return disk_write_sectors(FAT_LBA_START + FAT_RESERVED_SECTORS, FAT_SECTORS_PER_FAT, fat_buf);
}

static int flush_root(void) {
    return disk_write_sectors(root_lba(), FAT_ROOT_DIR_SECTORS, root_buf);
}

static int fat_format(void) {
    if (write_boot_sector() != 0) {
        return -1;
    }

    memset(fat_buf, 0, sizeof(fat_buf));
    fat_set(0, 0xFFF8);
    fat_set(1, FAT16_EOC);
    if (flush_fat() != 0) {
        return -1;
    }

    memset(root_buf, 0, sizeof(root_buf));
    if (flush_root() != 0) {
        return -1;
    }

    return 0;
}

static int valid_boot(const fat16_boot_sector_t *bs) {
    if (bs->signature != 0xAA55) {
        return 0;
    }
    if (bs->bytes_per_sector != 512) {
        return 0;
    }
    if (bs->sectors_per_cluster != FAT_SECTORS_PER_CLUSTER) {
        return 0;
    }
    if (bs->reserved_sector_count != FAT_RESERVED_SECTORS) {
        return 0;
    }
    if (bs->num_fats != FAT_FAT_COUNT) {
        return 0;
    }
    if (bs->root_entry_count != FAT_ROOT_ENTRIES) {
        return 0;
    }
    if (bs->total_sectors_16 != FAT_TOTAL_SECTORS) {
        return 0;
    }
    if (bs->fat_size_16 != FAT_SECTORS_PER_FAT) {
        return 0;
    }
    return 1;
}

static int to_upper_char(char c) {
    if (c >= 'a' && c <= 'z') {
        return c - 32;
    }
    return c;
}

static int is_valid_file_char(char c) {
    if (c >= 'A' && c <= 'Z') return 1;
    if (c >= '0' && c <= '9') return 1;
    if (c == '_' || c == '-' || c == '$') return 1;
    return 0;
}

static int fat_name_from_input(const char *in, char out[11]) {
    memset(out, ' ', 11);

    int i = 0;
    int base_len = 0;
    while (in[i] && in[i] != '.') {
        char c = (char)to_upper_char(in[i]);
        if (!is_valid_file_char(c) || base_len >= 8) {
            return -1;
        }
        out[base_len++] = c;
        i++;
    }

    if (base_len == 0) {
        return -1;
    }

    if (in[i] == '.') {
        i++;
        int ext_len = 0;
        while (in[i]) {
            char c = (char)to_upper_char(in[i]);
            if (!is_valid_file_char(c) || ext_len >= 3) {
                return -1;
            }
            out[8 + ext_len] = c;
            ext_len++;
            i++;
        }
    }

    return 0;
}

static void fat_name_to_printable(const fat_dir_entry_t *e, char out[FS_MAX_NAME + 1]) {
    int p = 0;
    for (int i = 0; i < 8 && e->name[i] != ' '; i++) {
        if (p < FS_MAX_NAME) {
            out[p++] = e->name[i];
        }
    }

    if (e->ext[0] != ' ') {
        if (p < FS_MAX_NAME) {
            out[p++] = '.';
        }
        for (int i = 0; i < 3 && e->ext[i] != ' '; i++) {
            if (p < FS_MAX_NAME) {
                out[p++] = e->ext[i];
            }
        }
    }

    out[p] = '\0';
}

static int valid_name(const char *name) {
    char tmp[11];
    return fat_name_from_input(name, tmp) == 0;
}

static fat_dir_entry_t *root_entries(void) {
    return (fat_dir_entry_t *)root_buf;
}

static int find_entry(const char *name, int *free_idx) {
    char f11[11];
    if (fat_name_from_input(name, f11) != 0) {
        return -1;
    }

    fat_dir_entry_t *ent = root_entries();
    int first_free = -1;

    for (int i = 0; i < FS_MAX_FILES; i++) {
        uint8_t lead = (uint8_t)ent[i].name[0];
        if (lead == 0x00 || lead == 0xE5) {
            if (first_free < 0) {
                first_free = i;
            }
            if (lead == 0x00) {
                break;
            }
            continue;
        }
        if (ent[i].attr == 0x0F) {
            continue;
        }
        if (memcmp(ent[i].name, f11, 8) == 0 && memcmp(ent[i].ext, f11 + 8, 3) == 0) {
            if (free_idx) {
                *free_idx = first_free;
            }
            return i;
        }
    }

    if (free_idx) {
        *free_idx = first_free;
    }
    return -1;
}

static uint16_t alloc_cluster(void) {
    for (uint16_t c = FAT_CLUSTER_MIN; c <= FAT_CLUSTER_MAX; c++) {
        if (fat_get(c) == 0x0000) {
            fat_set(c, FAT16_EOC);
            return c;
        }
    }
    return 0;
}

static void free_chain(uint16_t first) {
    uint16_t c = first;
    while (c >= FAT_CLUSTER_MIN && c <= FAT_CLUSTER_MAX) {
        uint16_t next = fat_get(c);
        fat_set(c, 0x0000);
        if (next >= 0xFFF8 || next == 0x0000) {
            break;
        }
        c = next;
    }
}

static int load_cluster_chain(uint16_t first, uint32_t size) {
    if (size > FS_MAX_FILE_SIZE) {
        size = FS_MAX_FILE_SIZE;
    }

    uint32_t copied = 0;
    uint16_t c = first;

    while (c >= FAT_CLUSTER_MIN && c <= FAT_CLUSTER_MAX && copied < size) {
        uint8_t sec[512];
        if (disk_read_sectors(cluster_lba(c), 1, sec) != 0) {
            return -1;
        }

        uint32_t take = size - copied;
        if (take > 512) {
            take = 512;
        }
        memcpy(read_cache + copied, sec, take);
        copied += take;

        uint16_t next = fat_get(c);
        if (next >= 0xFFF8 || next == 0x0000) {
            break;
        }
        c = next;
    }

    read_cache[copied] = '\0';
    return (int)copied;
}

static int write_cluster_chain(const char *data, uint32_t size, uint16_t *first_out) {
    uint16_t first = 0;
    uint16_t prev = 0;
    uint32_t written = 0;

    if (size == 0) {
        *first_out = 0;
        return 0;
    }

    while (written < size) {
        uint16_t c = alloc_cluster();
        if (c == 0) {
            if (first) {
                free_chain(first);
            }
            return -1;
        }

        if (!first) {
            first = c;
        }
        if (prev) {
            fat_set(prev, c);
        }
        fat_set(c, FAT16_EOC);

        uint8_t sec[512];
        memset(sec, 0, sizeof(sec));
        uint32_t take = size - written;
        if (take > 512) {
            take = 512;
        }
        memcpy(sec, data + written, take);

        if (disk_write_sectors(cluster_lba(c), 1, sec) != 0) {
            free_chain(first);
            return -1;
        }

        written += take;
        prev = c;
    }

    *first_out = first;
    return 0;
}

void fs_init(void) {
    fs_ready = 0;

    fat16_boot_sector_t bs;
    if (disk_read_sectors(FAT_LBA_START, 1, &bs) != 0 || !valid_boot(&bs)) {
        if (fat_format() != 0) {
            return;
        }
    }

    if (disk_read_sectors(FAT_LBA_START + FAT_RESERVED_SECTORS, FAT_SECTORS_PER_FAT, fat_buf) != 0) {
        return;
    }
    if (disk_read_sectors(root_lba(), FAT_ROOT_DIR_SECTORS, root_buf) != 0) {
        return;
    }

    fs_ready = 1;
}

int fs_touch(const char *name) {
    if (!fs_ready || !valid_name(name)) {
        return -1;
    }

    int free_idx = -1;
    int idx = find_entry(name, &free_idx);
    if (idx >= 0) {
        return 0;
    }
    if (free_idx < 0) {
        return -2;
    }

    char f11[11];
    fat_name_from_input(name, f11);
    fat_dir_entry_t *ent = root_entries();

    memset(&ent[free_idx], 0, sizeof(fat_dir_entry_t));
    memcpy(ent[free_idx].name, f11, 8);
    memcpy(ent[free_idx].ext, f11 + 8, 3);
    ent[free_idx].attr = FAT_ATTR_ARCHIVE;
    ent[free_idx].fst_clus_lo = 0;
    ent[free_idx].file_size = 0;

    return flush_root();
}

int fs_remove(const char *name) {
    if (!fs_ready) {
        return -1;
    }

    int idx = find_entry(name, 0);
    if (idx < 0) {
        return -1;
    }

    fat_dir_entry_t *ent = root_entries();
    uint16_t first = ent[idx].fst_clus_lo;
    if (first >= FAT_CLUSTER_MIN) {
        free_chain(first);
    }

    ent[idx].name[0] = (char)0xE5;
    ent[idx].file_size = 0;
    ent[idx].fst_clus_lo = 0;

    if (flush_fat() != 0) {
        return -2;
    }
    if (flush_root() != 0) {
        return -2;
    }

    return 0;
}

int fs_write_raw(const char *name, const char *data, size_t len) {
    if (!fs_ready || !valid_name(name) || len > FS_MAX_FILE_SIZE) {
        return -1;
    }

    int free_idx = -1;
    int idx = find_entry(name, &free_idx);
    fat_dir_entry_t *ent = root_entries();

    if (idx < 0) {
        if (free_idx < 0) {
            return -2;
        }

        char f11[11];
        fat_name_from_input(name, f11);
        idx = free_idx;
        memset(&ent[idx], 0, sizeof(fat_dir_entry_t));
        memcpy(ent[idx].name, f11, 8);
        memcpy(ent[idx].ext, f11 + 8, 3);
        ent[idx].attr = FAT_ATTR_ARCHIVE;
    }

    if (ent[idx].fst_clus_lo >= FAT_CLUSTER_MIN) {
        free_chain(ent[idx].fst_clus_lo);
        ent[idx].fst_clus_lo = 0;
    }

    uint16_t first = 0;
    if (write_cluster_chain(data, (uint32_t)len, &first) != 0) {
        return -3;
    }

    ent[idx].fst_clus_lo = first;
    ent[idx].file_size = (uint32_t)len;

    if (flush_fat() != 0) {
        return -4;
    }
    if (flush_root() != 0) {
        return -4;
    }

    return 0;
}

int fs_write(const char *name, const char *text) {
    return fs_write_raw(name, text, strlen(text));
}

int fs_append(const char *name, const char *text) {
    if (!fs_ready) {
        return -1;
    }

    size_t old_len = 0;
    const char *old = fs_read_ptr(name, &old_len);
    if (!old) {
        old_len = 0;
    }

    size_t add = strlen(text);
    if (old_len + add > FS_MAX_FILE_SIZE) {
        return -1;
    }

    memcpy(read_cache, old ? old : "", old_len);
    memcpy(read_cache + old_len, text, add);
    read_cache[old_len + add] = '\0';

    return fs_write_raw(name, read_cache, old_len + add);
}

const char *fs_read_ptr(const char *name, size_t *len) {
    if (!fs_ready) {
        return 0;
    }

    int idx = find_entry(name, 0);
    if (idx < 0) {
        return 0;
    }

    fat_dir_entry_t *ent = root_entries();
    uint32_t size = ent[idx].file_size;
    uint16_t first = ent[idx].fst_clus_lo;

    if (size == 0 || first < FAT_CLUSTER_MIN) {
        read_cache[0] = '\0';
        if (len) {
            *len = 0;
        }
        return read_cache;
    }

    int got = load_cluster_chain(first, size);
    if (got < 0) {
        return 0;
    }

    if (len) {
        *len = (size_t)got;
    }
    return read_cache;
}

int fs_list_entry(size_t index, const char **name, size_t *len) {
    if (!fs_ready) {
        return 0;
    }

    fat_dir_entry_t *ent = root_entries();
    size_t seen = 0;
    static char printable[FS_MAX_NAME + 1];

    for (size_t i = 0; i < FS_MAX_FILES; i++) {
        uint8_t lead = (uint8_t)ent[i].name[0];
        if (lead == 0x00) {
            break;
        }
        if (lead == 0xE5 || ent[i].attr == 0x0F) {
            continue;
        }

        if (seen == index) {
            fat_name_to_printable(&ent[i], printable);
            if (name) {
                *name = printable;
            }
            if (len) {
                *len = ent[i].file_size;
            }
            return 1;
        }
        seen++;
    }

    return 0;
}
