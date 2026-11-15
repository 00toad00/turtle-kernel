#pragma once

#include <stdint.h>

typedef struct {
    uint64_t    start;
    uint64_t    end;
    uint8_t     byte;
} __attribute__((packed)) mmap;