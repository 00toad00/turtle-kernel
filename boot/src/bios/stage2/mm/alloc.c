#include <mm/alloc.h>
#include <string.h>

alloc_map map;

void alloc_init(alloc_map *m, e820_entry *e) {
    uint32_t max_pages = sizeof(m->map) * 8;
    m->base  = 0;
    m->pages = 0;

    for (int i = 0; i < m->entries; i++) {
        if (e[i].base < (uint64_t)m->min || e[i].base >> 32 || e[i].type != 1) { continue; }

        uint32_t pages = (uint32_t)e[i].length / m->page_size;
        if (pages > m->pages) {
            m->base  = (uint32_t)e[i].base;
            m->pages = pages;
        }
    }

    if (m->pages > max_pages) {           
        m->base += (m->pages - max_pages) * m->page_size;
        m->pages = max_pages;
    }

    memset(m->map, 0, sizeof(m->map));
}

static int  bit_get(alloc_map *m, uint32_t p) { return (m->map[p >> 3] >> (p & 7)) & 1; }
static void bit_set(alloc_map *m, uint32_t p) { m->map[p >> 3] |=  (uint8_t)(1u << (p & 7)); }
static void bit_clr(alloc_map *m, uint32_t p) { m->map[p >> 3] &= (uint8_t)~(1u << (p & 7)); }

void *alloc_page(int count) {
    if (count <= 0 || (uint32_t)count > map.pages) return NULL;

    uint32_t run = 0;                       
    for (uint32_t p = 0; p < map.pages; p++) {
        if (bit_get(&map, p)) { run = 0; continue; }
        if (++run == (uint32_t)count) {
            uint32_t first = p + 1 - run;
            for (uint32_t k = first; k <= p; k++) bit_set(&map, k);
            return (void *)(uintptr_t)(map.base + first * map.page_size);
        }
    }
    return NULL;
}

void free_page(void *ptr, int count) {
    uint32_t addr = (uint32_t)(uintptr_t)ptr;
    if (addr < map.base || count <= 0) return;
    uint32_t first = (addr - map.base) / map.page_size;
    for (uint32_t k = first; k < first + (uint32_t)count && k < map.pages; k++)
        bit_clr(&map, k);
}