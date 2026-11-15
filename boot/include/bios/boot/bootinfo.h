#pragma once

#include <stdint.h>

typedef struct {
    uint64_t    lfb;
    uint32_t    width;
    uint32_t    height;
    uint32_t    pitch;
    uint8_t     bpp;
} __attribute__((packed)) bootinfo_video;

typedef struct {
    uint64_t    phys;
    uint64_t    virt;
} __attribute__((packed)) bootinfo_kernel;

typedef struct {
    uint64_t        status;
    bootinfo_video  video;
    bootinfo_kernel kernel;

    uint64_t        rsdp;
    uint64_t        efi_system_table;
    uint8_t         firmware;
} __attribute__((packed)) bootinfo;