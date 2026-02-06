#ifndef DISK_H
#define DISK_H

#include <stddef.h>
#include <stdint.h>

void disk_init(void);
int disk_read_sectors(uint32_t lba, uint8_t count, void *buf);
int disk_write_sectors(uint32_t lba, uint8_t count, const void *buf);

#endif
