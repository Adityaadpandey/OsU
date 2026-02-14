/*
 * fat32.h - FAT32 Filesystem Driver
 */

#ifndef FAT32_H
#define FAT32_H

#include <stdint.h>
#include <stddef.h>

/* FAT32 Boot Sector / BPB (BIOS Parameter Block) */
typedef struct {
    uint8_t  jmp[3];           /* Jump instruction */
    uint8_t  oem[8];           /* OEM identifier */
    uint16_t bytes_per_sector;
    uint8_t  sectors_per_cluster;
    uint16_t reserved_sectors;
    uint8_t  num_fats;
    uint16_t root_entry_count; /* 0 for FAT32 */
    uint16_t total_sectors_16; /* 0 for FAT32 */
    uint8_t  media_type;
    uint16_t fat_size_16;      /* 0 for FAT32 */
    uint16_t sectors_per_track;
    uint16_t num_heads;
    uint32_t hidden_sectors;
    uint32_t total_sectors_32;

    /* FAT32 Extended Boot Record */
    uint32_t fat_size_32;
    uint16_t ext_flags;
    uint16_t fs_version;
    uint32_t root_cluster;
    uint16_t fs_info_sector;
    uint16_t backup_boot_sector;
    uint8_t  reserved[12];
    uint8_t  drive_number;
    uint8_t  reserved1;
    uint8_t  boot_signature;
    uint32_t volume_id;
    uint8_t  volume_label[11];
    uint8_t  fs_type[8];
} __attribute__((packed)) fat32_bpb_t;

/* Directory Entry (32 bytes) */
typedef struct {
    uint8_t  name[11];         /* 8.3 filename */
    uint8_t  attr;             /* File attributes */
    uint8_t  nt_reserved;
    uint8_t  create_time_tenth;
    uint16_t create_time;
    uint16_t create_date;
    uint16_t access_date;
    uint16_t cluster_hi;       /* High 16 bits of cluster */
    uint16_t modify_time;
    uint16_t modify_date;
    uint16_t cluster_lo;       /* Low 16 bits of cluster */
    uint32_t file_size;
} __attribute__((packed)) fat32_dir_entry_t;

/* File attributes */
#define FAT_ATTR_READ_ONLY  0x01
#define FAT_ATTR_HIDDEN     0x02
#define FAT_ATTR_SYSTEM     0x04
#define FAT_ATTR_VOLUME_ID  0x08
#define FAT_ATTR_DIRECTORY  0x10
#define FAT_ATTR_ARCHIVE    0x20
#define FAT_ATTR_LFN        0x0F

/* FAT entry special values */
#define FAT32_EOC       0x0FFFFFF8  /* End of cluster chain */
#define FAT32_BAD       0x0FFFFFF7  /* Bad cluster */
#define FAT32_FREE      0x00000000  /* Free cluster */

/* FAT32 driver state */
typedef struct {
    int valid;
    uint32_t fat_start_lba;
    uint32_t cluster_start_lba;
    uint32_t root_cluster;
    uint32_t sectors_per_cluster;
    uint32_t bytes_per_sector;
    uint32_t fat_size;
} fat32_t;

/* Initialize FAT32 driver for a partition */
int fat32_init(uint32_t partition_lba);

/* List directory contents */
int fat32_list_dir(uint32_t cluster, void (*callback)(const char *name, int is_dir, uint32_t size));

/* Read file into buffer */
int fat32_read_file(uint32_t cluster, uint8_t *buffer, size_t max_size);

/* Find file/directory by name in directory cluster */
int fat32_find_entry(uint32_t dir_cluster, const char *name, fat32_dir_entry_t *entry);

/* Get cluster number from directory entry */
static inline uint32_t fat32_get_cluster(const fat32_dir_entry_t *entry) {
    return ((uint32_t)entry->cluster_hi << 16) | entry->cluster_lo;
}

#endif /* FAT32_H */
