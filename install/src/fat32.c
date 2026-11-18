#include <stdint.h>
#include <string.h>
#include <errno.h>

#include <block.h>
#include <fat32.h>

#define RESERVED_SECTORS 32
#define FAT_COUNT        2
#define ROOT_CLUSTER     2
#define MIN_CLUSTERS     65525
#define LABEL            "TURTLE     "

_Static_assert(sizeof(fat32_boot_sector) == SECTOR_SIZE, "boot sector must be one sector");
_Static_assert(sizeof(fat32_fsinfo) == SECTOR_SIZE, "FSInfo must be one sector");

// Cluster size by volume size, from Microsoft's FAT specification (fatgen103)
static uint8_t sectors_per_cluster(uint32_t sectors) {
    if (sectors <=   532480) return 1;   // up to 260 MiB:   512 B
    if (sectors <= 16777216) return 8;   // up to 8 GiB:   4 KiB
    if (sectors <= 33554432) return 16;  // up to 16 GiB:  8 KiB
    if (sectors <= 67108864) return 32;  // up to 32 GiB: 16 KiB
    return 64;                           // larger:       32 KiB
}

// Formats sectors [lba, lba + sectors) as an empty FAT32 volume.
// Returns 0, or -1 with errno set.
int fat32_format(int fd, uint32_t lba, uint32_t sectors, uint32_t volume_id) {
    uint8_t spc = sectors_per_cluster(sectors);

    // FAT size, as computed in fatgen103 (may overestimate by a sector or so)
    uint32_t tmp1   = sectors - RESERVED_SECTORS;
    uint32_t tmp2   = (256 * spc + FAT_COUNT) / 2;
    uint32_t fat_sz = (tmp1 + tmp2 - 1) / tmp2;

    uint32_t data_lba = lba + RESERVED_SECTORS + FAT_COUNT * fat_sz;
    uint32_t clusters = (sectors - RESERVED_SECTORS - FAT_COUNT * fat_sz) / spc;
    if (sectors < FAT32_MIN_SECTORS || clusters < MIN_CLUSTERS) { errno = EINVAL; return -1; }

    // Clear the reserved area, both FATs and the root directory cluster
    if (zero_sectors(fd, lba, (uint64_t)(data_lba - lba) + spc) != 0) return -1;

    fat32_boot_sector bs; memset(&bs, 0, sizeof(bs));
    memcpy(bs.jmp, "\xEB\x58\x90", 3);
    memcpy(bs.oem, "MSWIN4.1", 8);
    bs.bytes_per_sector     = SECTOR_SIZE;
    bs.sectors_per_cluster  = spc;
    bs.reserved_sectors     = RESERVED_SECTORS;
    bs.fat_count            = FAT_COUNT;
    bs.media                = 0xF8;
    bs.sectors_per_track    = 63;
    bs.heads                = 255;
    bs.hidden_sectors       = lba;
    bs.total_sectors32      = sectors;
    bs.fat_size32           = fat_sz;
    bs.root_cluster         = ROOT_CLUSTER;
    bs.fsinfo_sector        = 1;
    bs.backup_boot_sector   = 6;
    bs.drive_number         = 0x80;
    bs.boot_signature       = 0x29;
    bs.volume_id            = volume_id;
    memcpy(bs.volume_label, LABEL, 11);
    memcpy(bs.fs_type, "FAT32   ", 8);
    bs.signature            = 0xAA55;

    fat32_fsinfo fsi; memset(&fsi, 0, sizeof(fsi));
    fsi.lead_sig    = 0x41615252;
    fsi.struct_sig  = 0x61417272;
    fsi.free_count  = clusters - 1;         // the root directory takes one
    fsi.next_free   = ROOT_CLUSTER + 1;
    fsi.trail_sig   = 0xAA550000;

    // FAT[0] = media, FAT[1] = end of chain, FAT[2] = root directory (one cluster)
    uint32_t fat[SECTOR_SIZE / 4] = { 0x0FFFFFF8, 0x0FFFFFFF, 0x0FFFFFFF };

    fat_dir_entry root[SECTOR_SIZE / sizeof(fat_dir_entry)]; memset(root, 0, sizeof(root));
    memcpy(root[0].name, LABEL, 11);
    root[0].attr = FAT_ATTR_VOLUME_ID;

    for (uint32_t i = 0; i < FAT_COUNT; i++)
        if (write_sectors(fd, lba + RESERVED_SECTORS + i * fat_sz, fat, 1) != 0) return -1;
    if (write_sectors(fd, data_lba, root, 1) != 0) return -1;

    // Boot sectors last, so an interrupted format doesn't look like a valid volume
    if (write_sectors(fd, lba + 1, &fsi, 1) != 0) return -1;
    if (write_sectors(fd, lba + 7, &fsi, 1) != 0) return -1;
    if (write_sectors(fd, lba + 6, &bs, 1) != 0) return -1;
    if (write_sectors(fd, lba, &bs, 1) != 0) return -1;
    return 0;
}
