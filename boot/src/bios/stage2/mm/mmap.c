#include <mm/e820.h>
#include <mm/mmap.h>

int mmap_add(mmap_entry *entry, int entries, uint64_t start, uint64_t size, uint8_t type) {
    if (size == 0) { return 0; }
    for (int i = 0; i < entries; i++) {
        if (entry[i].type != 0) { continue; }
        entry[i].start = start;
        entry[i].size = size;
        entry[i].type = type;
        break;
    }
    return -1;
}