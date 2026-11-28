#pragma once

#include <stdint.h>
#include <driver/header/driver.h>

typedef enum {
    BLKDV_BLKDEVICE = (1 << 0),
    BLKDV_READ      = (1 << 1),
} blk_dev_caps;

typedef struct blk_device {
    device      dev;
    uint8_t     disk_id;
    uint64_t    sectors;
    uint16_t    bps;

    void        *(*read)(struct blk_device *self, void *buf, uint64_t lba, uint32_t n);
} blk_device;