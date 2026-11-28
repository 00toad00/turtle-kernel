#pragma once

#include <stdint.h>

typedef struct {
    uint8_t     size;
    uint8_t     zero;
    uint16_t    n;
    uint16_t    off;
    uint16_t    seg;
    uint64_t    lba;
} __attribute__((packed)) disk_address_packet;

typedef struct {
    uint16_t    size;
    uint16_t    flags;
    uint32_t    c;
    uint32_t    h;
    uint32_t    s;
    uint64_t    sectors;
    uint16_t    sector_size;
} __attribute__((packed)) drive_params;