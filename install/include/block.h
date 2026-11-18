#pragma once

#include <stdint.h>

#define SECTOR_SIZE 512

int     read_sectors(int fd, uint64_t lba, void *buf, uint32_t count);
int     write_sectors(int fd, uint64_t lba, const void *buf, uint32_t count);
int     zero_sectors(int fd, uint64_t lba, uint64_t count);
int64_t file_sectors(const char *path);
