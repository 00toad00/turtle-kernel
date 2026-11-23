#include <mm/alloc.h>
#include <string.h>

alloc_map map;
malloc_entry *mptr;
uint32_t malloc_pages;

void alloc_init(alloc_map *m, e820_entry *e) {
    m->base = 0;
    m->pages = 0;

    for (int i = 0; i < m->entries; i++) {
        if (e[i].base < (uint64_t)m->min || e[i].base >> 32 || e[i].type != 1) { continue; }

        uint32_t pages = (uint32_t)e[i].length / m->page_size;
        if (pages > m->pages) {
            m->base = (uint32_t)e[i].base;
            m->pages = pages;
        }
    }

    uint32_t map_size = (m->pages+7)/8;
    uint32_t map_pages = (map_size + m->page_size - 1) / m->page_size;
    if (m->pages <= map_pages) { m->pages = 0; return; }

    m->map = (uint8_t*)m->base;
    m->base += map_pages*m->page_size;
    m->pages -= map_pages;

    uint32_t aligned = (m->base + 4095) & ~4095u;           // next 4 KiB boundary
    uint32_t skip    = (aligned - m->base) / m->page_size;  // blocks lost to alignment
    if (m->pages <= skip) { m->pages = 0; return; }
    m->base   = aligned;
    m->pages -= skip;

    memset(m->map, 0, map_size);
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

void malloc_init(uint32_t pages) {
    mptr = (malloc_entry*)alloc_page(pages);
    malloc_pages = pages;
    malloc_entry *tail = mptr + 1;
    mptr->prev = NULL;  mptr->next = tail;  mptr->size = 0;
    tail->prev = mptr;  tail->next = NULL;  tail->size = 0;
}

void *malloc(size_t n) {
    if (n == 0) return NULL;
    n = (n + 15) & ~(size_t)15;                     

    malloc_entry *ptr = mptr;
    while (ptr->next) { ptr = ptr->next; }

    char *data = (char *)(ptr + 1);
    malloc_entry *tail = (malloc_entry *)(data + n); 

    tail->prev = ptr;
    tail->next = NULL;
    tail->size = 0;

    ptr->size = (uint32_t)n;
    ptr->next = tail;
    return data;
}

void *calloc(size_t num, size_t size) {
    size_t n = num*size;
    n = (n + 15) & ~(size_t)15;
    void *ptr = malloc(n);
    if (ptr) { memset(ptr, 0, n); }
    return (ptr);
}

void free(void *p) {
    if (!p) { return; }
    malloc_entry *ptr = mptr;
    malloc_entry *e = (malloc_entry*)p - 1;
    while (ptr->next) {
        if (ptr->next == e) { break; }
        ptr = ptr->next;
    }
    if (ptr->next == NULL) { return; }

    // ptr - p - next
    malloc_entry *pnext = (malloc_entry*)e;
    pnext = pnext->next; 
    
    ptr->next = pnext;
    if (pnext) { pnext->prev = ptr; }
}