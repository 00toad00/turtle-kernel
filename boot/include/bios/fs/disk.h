#pragma once

#include <stdint.h>

extern uint32_t bdisk;

typedef struct {
    uint8_t     size;
    uint8_t     zero;
    uint16_t    n;
    uint16_t    off;
    uint16_t    seg;
    uint64_t    lba;
} __attribute__((packed)) disk_address_packet;

int read(void *buf, uint64_t lba, uint32_t n);