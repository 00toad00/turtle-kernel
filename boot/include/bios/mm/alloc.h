#pragma once

#include <mm/e820.h>
#include <stdint.h>
#include <stddef.h>

typedef struct {
    e820_entry  *e820;
    int         entries;
    uint32_t    base;
    uint32_t    min;
    uint32_t    pages;
    uint32_t    page_size;
    uint8_t     *map;
} alloc_map;

typedef struct {
    void        *prev;
    void        *next;
    uint32_t    size;
    uint32_t    reserved;
} malloc_entry;

extern alloc_map map;
extern char __boot_end;

void alloc_init(alloc_map *m, e820_entry* e);
void *alloc_page(int count);
void free_page(void *ptr, int count);

void malloc_init(uint32_t pages);
void *malloc(size_t n);
void *calloc(size_t num, size_t size);
void free(void *p);