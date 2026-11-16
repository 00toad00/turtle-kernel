#include <fs/disk.h>
#include <fs/mbr.h>

uint64_t find_esp() {
    mbr m;
    if(read(&m, 0, 1) != 0) { return 0; }

    if (m.signature != 0xAA55) { return 0; }

    int is_mbr = 1;
    for (int i = 0; i < 4; i++) {
        if(m.entry[i].type == MBR_GPT) { is_mbr = 0; }
    }

    if (is_mbr) {
        for (int i = 0; i < 4; i++) {
            if (m.entry[i].type == MBR_ESP && m.entry[i].size != 0) { return m.entry[i].lba; }
        }
        return 0;
    } else {
        if (read(&m, 1, 1) != 0) { return 0; }
        /* GPT LOGIC */
    }

    return 0;
}