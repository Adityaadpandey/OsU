#include "disk.h"

#include <stdint.h>

#include "io.h"

#define ATA_IO_BASE 0x1F0
#define ATA_REG_DATA (ATA_IO_BASE + 0)
#define ATA_REG_SECCOUNT (ATA_IO_BASE + 2)
#define ATA_REG_LBA0 (ATA_IO_BASE + 3)
#define ATA_REG_LBA1 (ATA_IO_BASE + 4)
#define ATA_REG_LBA2 (ATA_IO_BASE + 5)
#define ATA_REG_DRIVE (ATA_IO_BASE + 6)
#define ATA_REG_STATUS (ATA_IO_BASE + 7)
#define ATA_REG_COMMAND (ATA_IO_BASE + 7)
#define ATA_REG_ALTSTATUS 0x3F6

#define ATA_SR_ERR 0x01
#define ATA_SR_DRQ 0x08
#define ATA_SR_DF 0x20
#define ATA_SR_BSY 0x80

#define ATA_CMD_READ 0x20
#define ATA_CMD_WRITE 0x30

static void ata_delay(void) {
    inb(ATA_REG_ALTSTATUS);
    inb(ATA_REG_ALTSTATUS);
    inb(ATA_REG_ALTSTATUS);
    inb(ATA_REG_ALTSTATUS);
}

static int ata_poll(void) {
    uint8_t status;
    do {
        status = inb(ATA_REG_STATUS);
    } while (status & ATA_SR_BSY);

    if (status & (ATA_SR_ERR | ATA_SR_DF)) {
        return -1;
    }

    while (!(status & ATA_SR_DRQ)) {
        status = inb(ATA_REG_STATUS);
        if (status & (ATA_SR_ERR | ATA_SR_DF)) {
            return -1;
        }
    }

    return 0;
}

static void ata_select_lba(uint32_t lba, uint8_t count) {
    outb(ATA_REG_DRIVE, (uint8_t)(0xE0 | ((lba >> 24) & 0x0F)));
    outb(ATA_REG_SECCOUNT, count);
    outb(ATA_REG_LBA0, (uint8_t)(lba & 0xFF));
    outb(ATA_REG_LBA1, (uint8_t)((lba >> 8) & 0xFF));
    outb(ATA_REG_LBA2, (uint8_t)((lba >> 16) & 0xFF));
}

void disk_init(void) {
}

int disk_read_sectors(uint32_t lba, uint8_t count, void *buf) {
    uint16_t *dst = (uint16_t *)buf;

    if (count == 0) {
        return 0;
    }

    ata_select_lba(lba, count);
    outb(ATA_REG_COMMAND, ATA_CMD_READ);

    for (uint8_t s = 0; s < count; s++) {
        if (ata_poll() != 0) {
            return -1;
        }

        for (int i = 0; i < 256; i++) {
            dst[s * 256 + i] = inw(ATA_REG_DATA);
        }
        ata_delay();
    }

    return 0;
}

int disk_write_sectors(uint32_t lba, uint8_t count, const void *buf) {
    const uint16_t *src = (const uint16_t *)buf;

    if (count == 0) {
        return 0;
    }

    ata_select_lba(lba, count);
    outb(ATA_REG_COMMAND, ATA_CMD_WRITE);

    for (uint8_t s = 0; s < count; s++) {
        if (ata_poll() != 0) {
            return -1;
        }

        for (int i = 0; i < 256; i++) {
            outw(ATA_REG_DATA, src[s * 256 + i]);
        }
        ata_delay();
    }

    return 0;
}
