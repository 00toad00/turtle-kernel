#pragma once

#include <mm/e820.h>
#include <stdint.h>

typedef struct {
    e820_entry  *e820;
    int         entries;
    uint32_t    base;
    uint32_t    min;
    uint32_t    pages;
    uint32_t    page_size;
    uint8_t     map[512];
} alloc_map;

typedef struct {
    uint32_t    ptr;
    uint32_t    size;    
} malloc_entry;

extern alloc_map map;
extern char __boot_end;

void alloc_init(alloc_map *m, e820_entry* e);
void *alloc_page(int count);
void free_page(void *ptr, int count);