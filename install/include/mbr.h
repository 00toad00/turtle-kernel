#pragma once

#include <stdint.h>

enum {
    MBR_EMPTY   = 0x00,
    MBR_NONFS   = 0xDA,
    MBR_GPT     = 0xEE,
    MBR_ESP     = 0xEF
};

typedef struct {
    uint8_t     status;
    uint8_t     chs_first[3];
    uint8_t     type;
    uint8_t     chs_last[3];
    uint32_t    lba;
    uint32_t    size;
} __attribute__((packed)) mbr_entry;

typedef struct {
    uint8_t     code[440];
    uint32_t    disk_signature;
    uint16_t    reserved;
    mbr_entry   entry[4];
    uint16_t    signature;
} __attribute__((packed)) mbr;