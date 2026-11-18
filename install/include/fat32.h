#pragma once

#include <stdint.h>

// Below this many sectors a volume can't hold the 65525 clusters FAT32 needs
#define FAT32_MIN_SECTORS 66600

typedef struct {
    uint8_t  jmp[3];
    char     oem[8];
    uint16_t bytes_per_sector;
    uint8_t  sectors_per_cluster;
    uint16_t reserved_sectors;
    uint8_t  fat_count;
    uint16_t root_entries;
    uint16_t total_sectors16;
    uint8_t  media;
    uint16_t fat_size16;
    uint16_t sectors_per_track;
    uint16_t heads;
    uint32_t hidden_sectors;
    uint32_t total_sectors32;
    uint32_t fat_size32;
    uint16_t ext_flags;
    uint16_t fs_version;
    uint32_t root_cluster;
    uint16_t fsinfo_sector;
    uint16_t backup_boot_sector;
    uint8_t  reserved[12];
    uint8_t  drive_number;
    uint8_t  reserved1;
    uint8_t  boot_signature;
    uint32_t volume_id;
    char     volume_label[11];
    char     fs_type[8];
    uint8_t  boot_code[420];
    uint16_t signature;
} __attribute__((packed)) fat32_boot_sector;

typedef struct {
    uint32_t lead_sig;       // 0x41615252
    uint8_t  reserved[480];
    uint32_t struct_sig;     // 0x61417272
    uint32_t free_count;
    uint32_t next_free;
    uint8_t  reserved1[12];
    uint32_t trail_sig;      // 0xAA550000
} __attribute__((packed)) fat32_fsinfo;

typedef struct {
    char     name[11];
    uint8_t  attr;
    uint8_t  nt_reserved;
    uint8_t  create_time_tenth;
    uint16_t create_time;
    uint16_t create_date;
    uint16_t access_date;
    uint16_t cluster_high;
    uint16_t write_time;
    uint16_t write_date;
    uint16_t cluster_low;
    uint32_t file_size;
} __attribute__((packed)) fat_dir_entry;

#define FAT_ATTR_VOLUME_ID 0x08

int fat32_format(int fd, uint32_t lba, uint32_t sectors, uint32_t volume_id);
