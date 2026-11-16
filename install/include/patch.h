#pragma once

#include <stdint.h>

#define MBR_PATCH_OFF 420

typedef struct {
    uint32_t lba;
    uint16_t sec;
} __attribute__((packed)) patch;