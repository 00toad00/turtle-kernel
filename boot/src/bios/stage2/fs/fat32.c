#include <fs/fat32.h>
#include <fs/disk.h>
#include <stddef.h>

uint64_t sector_of_cluster(fat_volume *vol, uint32_t n) {
    return vol->lba
            + vol->bpb.reserved_sectors 
            + vol->bpb.fat_count*vol->bpb.fat_size32
            + (n - 2)*vol->bpb.sectors_per_cluster;
}

uint32_t cluster_per_chain(fat_volume *vol, uint32_t n) {
    uint32_t clusters = 1;
    for (uint32_t i = 0; i < vol->epf; i++) {
        if (FAT_ENTRY_END(FAT_ENTRY_MASK(vol->fat[n]))) { return clusters; }
        n = FAT_ENTRY_MASK(vol->fat[n]);
        clusters += 1;
    }
    return 0;
}

void *read_cluster(fat_volume *vol, uint32_t n, void *buf) {
    uint64_t lba = sector_of_cluster(vol, n);
    uint8_t spc = vol->bpb.sectors_per_cluster;
    return read(buf, lba, spc);
}

void *read_chain(fat_volume *vol, uint32_t n, void *buf) {
    char *ptr = (char*)buf;
    uint8_t spc = vol->bpb.sectors_per_cluster;
    uint16_t bps = vol->bpb.bytes_per_sector;
    for (uint32_t i = 0; i < vol->epf; i++) {
        if (FAT_ENTRY_END(FAT_ENTRY_MASK(vol->fat[n]))) { 
            if(!read_cluster(vol, n, ptr)) { return NULL; }
            return buf; 
        }
        if(!read_cluster(vol, n, ptr)) { return NULL; }
        ptr += spc*bps;
        n = FAT_ENTRY_MASK(vol->fat[n]);
    }
    return 0;
}