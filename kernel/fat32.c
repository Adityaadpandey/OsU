/*
 * fat32.c - FAT32 Filesystem Implementation
 */

#include "fat32.h"
#include "disk.h"
#include "string.h"
#include "vga.h"

static fat32_t fs;
static uint8_t sector_buffer[512];
static uint8_t cluster_buffer[4096];  /* Max 8 sectors per cluster */

int fat32_init(uint32_t partition_lba) {
    /* Read boot sector */
    if (disk_read_sectors(partition_lba, 1, sector_buffer) != 0) {
        return -1;
    }

    fat32_bpb_t *bpb = (fat32_bpb_t *)sector_buffer;

    /* Validate FAT32 signature */
    if (bpb->bytes_per_sector != 512) {
        return -1;  /* We only support 512-byte sectors */
    }

    if (bpb->num_fats == 0 || bpb->fat_size_32 == 0) {
        return -1;  /* Not FAT32 */
    }

    /* Calculate filesystem parameters */
    fs.bytes_per_sector = bpb->bytes_per_sector;
    fs.sectors_per_cluster = bpb->sectors_per_cluster;
    fs.fat_size = bpb->fat_size_32;
    fs.root_cluster = bpb->root_cluster;

    /* Calculate LBAs */
    fs.fat_start_lba = partition_lba + bpb->reserved_sectors;
    fs.cluster_start_lba = fs.fat_start_lba + (bpb->num_fats * bpb->fat_size_32);

    fs.valid = 1;
    return 0;
}

/* Convert cluster number to LBA */
static uint32_t cluster_to_lba(uint32_t cluster) {
    return fs.cluster_start_lba + (cluster - 2) * fs.sectors_per_cluster;
}

/* Read FAT entry for a cluster */
static uint32_t fat_read_entry(uint32_t cluster) {
    uint32_t fat_offset = cluster * 4;
    uint32_t fat_sector = fs.fat_start_lba + (fat_offset / 512);
    uint32_t entry_offset = fat_offset % 512;

    if (disk_read_sectors(fat_sector, 1, sector_buffer) != 0) {
        return FAT32_BAD;
    }

    uint32_t entry = *(uint32_t *)(sector_buffer + entry_offset);
    return entry & 0x0FFFFFFF;  /* Only 28 bits used */
}

/* Read entire cluster into buffer */
static int read_cluster(uint32_t cluster, uint8_t *buffer) {
    uint32_t lba = cluster_to_lba(cluster);
    return disk_read_sectors(lba, fs.sectors_per_cluster, buffer);
}

/* Convert FAT 8.3 name to readable format */
static void fat_name_to_str(const uint8_t *fat_name, char *str) {
    int i = 0, j = 0;

    /* Copy name part (first 8 chars, trim spaces) */
    for (i = 0; i < 8 && fat_name[i] != ' '; i++) {
        str[j++] = fat_name[i];
    }

    /* Add dot and extension if present */
    if (fat_name[8] != ' ') {
        str[j++] = '.';
        for (i = 8; i < 11 && fat_name[i] != ' '; i++) {
            str[j++] = fat_name[i];
        }
    }

    str[j] = '\0';

    /* Convert to lowercase */
    for (i = 0; str[i]; i++) {
        if (str[i] >= 'A' && str[i] <= 'Z') {
            str[i] += 32;
        }
    }
}

int fat32_list_dir(uint32_t cluster, void (*callback)(const char *name, int is_dir, uint32_t size)) {
    if (!fs.valid) return -1;

    uint32_t current_cluster = cluster;

    while (current_cluster < FAT32_EOC) {
        if (read_cluster(current_cluster, cluster_buffer) != 0) {
            return -1;
        }

        fat32_dir_entry_t *entries = (fat32_dir_entry_t *)cluster_buffer;
        int entries_per_cluster = (fs.sectors_per_cluster * 512) / sizeof(fat32_dir_entry_t);

        for (int i = 0; i < entries_per_cluster; i++) {
            fat32_dir_entry_t *e = &entries[i];

            /* End of directory */
            if (e->name[0] == 0x00) {
                return 0;
            }

            /* Deleted entry */
            if (e->name[0] == 0xE5) {
                continue;
            }

            /* Skip LFN entries and volume label */
            if ((e->attr & FAT_ATTR_LFN) == FAT_ATTR_LFN ||
                (e->attr & FAT_ATTR_VOLUME_ID)) {
                continue;
            }

            /* Skip . and .. */
            if (e->name[0] == '.') {
                continue;
            }

            char name[13];
            fat_name_to_str(e->name, name);

            int is_dir = (e->attr & FAT_ATTR_DIRECTORY) != 0;
            callback(name, is_dir, e->file_size);
        }

        /* Get next cluster in chain */
        current_cluster = fat_read_entry(current_cluster);
    }

    return 0;
}

int fat32_read_file(uint32_t cluster, uint8_t *buffer, size_t max_size) {
    if (!fs.valid) return -1;

    size_t bytes_read = 0;
    uint32_t bytes_per_cluster = fs.sectors_per_cluster * 512;
    uint32_t current_cluster = cluster;

    while (current_cluster < FAT32_EOC && bytes_read < max_size) {
        if (read_cluster(current_cluster, cluster_buffer) != 0) {
            return -1;
        }

        size_t to_copy = bytes_per_cluster;
        if (bytes_read + to_copy > max_size) {
            to_copy = max_size - bytes_read;
        }

        memcpy(buffer + bytes_read, cluster_buffer, to_copy);
        bytes_read += to_copy;

        current_cluster = fat_read_entry(current_cluster);
    }

    return (int)bytes_read;
}

int fat32_find_entry(uint32_t dir_cluster, const char *name, fat32_dir_entry_t *entry) {
    if (!fs.valid) return -1;

    uint32_t current_cluster = dir_cluster;

    while (current_cluster < FAT32_EOC) {
        if (read_cluster(current_cluster, cluster_buffer) != 0) {
            return -1;
        }

        fat32_dir_entry_t *entries = (fat32_dir_entry_t *)cluster_buffer;
        int entries_per_cluster = (fs.sectors_per_cluster * 512) / sizeof(fat32_dir_entry_t);

        for (int i = 0; i < entries_per_cluster; i++) {
            fat32_dir_entry_t *e = &entries[i];

            if (e->name[0] == 0x00) {
                return -1;  /* Not found */
            }

            if (e->name[0] == 0xE5) continue;
            if ((e->attr & FAT_ATTR_LFN) == FAT_ATTR_LFN) continue;

            char entry_name[13];
            fat_name_to_str(e->name, entry_name);

            if (strcmp(entry_name, name) == 0) {
                *entry = *e;
                return 0;
            }
        }

        current_cluster = fat_read_entry(current_cluster);
    }

    return -1;
}
