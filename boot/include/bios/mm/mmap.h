#pragma once

#include <stdint.h>

enum {
    MMAP_FREE = 1,
    MMAP_RESERVED,
    MMAP_BOOT,
};

typedef struct {
    uint64_t    start;
    uint64_t    size;
    uint8_t     type;
} __attribute__((packed)) mmap_entry;