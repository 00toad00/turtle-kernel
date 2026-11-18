#include <stdint.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/random.h>
#include <linux/fs.h>

#include <block.h>
#include <config.h>
#include <fat32.h>
#include <mbr.h>
#include <patch.h>

#define STAGE1_PATH "build/stage-1.bin"
#define STAGE2_PATH "build/stage-2.bin"
#define STAGE2_LBA  2048

// Stage 1 loads stage 2 at 0x8000 and later sets the stack to 0x90000.
// Keep stage 2 well below that so it never overlaps the stack.
#define STAGE2_MAX_SECTORS ((0x80000 - 0x8000) / SECTOR_SIZE)

// 1 MiB is reserved for stage 2; the ESP follows, 1 MiB aligned, and takes
// the rest of the disk (up to what a 32-bit MBR entry can describe).
#define ESP_LBA (STAGE2_LBA + 2048)
_Static_assert(STAGE2_LBA + STAGE2_MAX_SECTORS <= ESP_LBA, "stage 2 would overlap the ESP");

static void usage(void) {
    fprintf(stderr,
        "usage: turtle-install --dev <device|image> [--mbr] [--yes]\n"
        "  --dev <path>  block device (e.g. /dev/sdX) or disk image file\n"
        "  --mbr         use an MBR partition table (default)\n"
        "  --yes         do not ask for confirmation\n"
        "Run from the repository root after `make`.\n");
}

static void heap_alloc_fail(void *ptr) {
    if (!ptr) {
        fprintf(stderr, "Heap allocation failed\n");
        exit(1);
    }
}

static int confirm(const char *dev, uint64_t bytes) {
    if (!isatty(STDIN_FILENO)) {
        fprintf(stderr, "stdin is not a terminal; pass --yes to confirm non-interactively\n");
        return 0;
    }
    printf("This will overwrite the partition table and format an ESP on\n"
           "  %s (%.1f MiB)\n"
           "All data on it will be lost. Type 'yes' to continue: ",
           dev, (double)bytes / (1024.0 * 1024.0));
    fflush(stdout);

    char line[16];
    if (!fgets(line, sizeof(line), stdin)) return 0;
    line[strcspn(line, "\n")] = '\0';
    return strcmp(line, "yes") == 0;
}

int main(int argc, char **argv) {
    argc--; argv++;

    config cfg = {0};

    for (int i = 0; i < argc; i++) {
        if (!strcmp(argv[i], "--mbr")) {
            if (cfg.part_type_set) {
                fprintf(stderr, "%s may be used only one time\n", argv[i]);
                return 1;
            }
            cfg.part_type = 0; cfg.part_type_set = true;
        } else if (!strcmp(argv[i], "--dev")) {
            if (i + 1 >= argc) {
                fprintf(stderr, "%s needs a device specified e.g. /dev/sdX\n", argv[i]);
                return 1;
            }
            cfg.dev = argv[++i];
        } else if (!strcmp(argv[i], "--yes") || !strcmp(argv[i], "-y")) {
            cfg.yes = true;
        } else if (!strcmp(argv[i], "--help") || !strcmp(argv[i], "-h")) {
            usage();
            return 0;
        } else {
            fprintf(stderr, "unknown argument: %s\n", argv[i]);
            usage();
            return 1;
        }
    }

    if (!cfg.dev) {
        fprintf(stderr, "Please specify a device to write to\n");
        usage();
        return 1;
    }
    if (cfg.part_type != 0) {
        fprintf(stderr, "only MBR is supported for now\n");
        return 1;
    }

    // ---- Load and validate the boot files (before touching the target) ----
    int64_t s1_sectors = file_sectors(STAGE1_PATH);
    int64_t s2_sectors = file_sectors(STAGE2_PATH);
    if (s1_sectors < 0) { perror(STAGE1_PATH); fprintf(stderr, "did you run `make`?\n"); return 1; }
    if (s2_sectors < 0) { perror(STAGE2_PATH); fprintf(stderr, "did you run `make`?\n"); return 1; }

    struct stat st1;
    if (stat(STAGE1_PATH, &st1) != 0 || st1.st_size != SECTOR_SIZE) {
        fprintf(stderr, "%s must be exactly %d bytes\n", STAGE1_PATH, SECTOR_SIZE);
        return 1;
    }
    if (s2_sectors == 0 || s2_sectors > STAGE2_MAX_SECTORS) {
        fprintf(stderr, "%s is %lld sectors; must be 1..%d\n",
                STAGE2_PATH, (long long)s2_sectors, STAGE2_MAX_SECTORS);
        return 1;
    }

    mbr m; memset(&m, 0, sizeof(mbr));
    int fd = open(STAGE1_PATH, O_RDONLY);
    if (fd < 0) { perror(STAGE1_PATH); return 1; }
    if (read_sectors(fd, 0, &m, 1) != 0) { perror(STAGE1_PATH); close(fd); return 1; }
    close(fd);

    // calloc so the tail of the last sector is zero, not leftover heap data
    uint8_t *s2 = calloc((size_t)s2_sectors, SECTOR_SIZE);
    heap_alloc_fail(s2);
    fd = open(STAGE2_PATH, O_RDONLY);
    if (fd < 0) { perror(STAGE2_PATH); return 1; }
    // Whole sectors may hit EOF partway through the last one; read the
    // full sectors, then whatever remains.
    struct stat st2;
    if (fstat(fd, &st2) != 0) { perror(STAGE2_PATH); return 1; }
    for (off_t done = 0; done < st2.st_size; ) {
        ssize_t r = pread(fd, s2 + done, (size_t)(st2.st_size - done), done);
        if (r < 0 && errno == EINTR) continue;
        if (r <= 0) { perror(STAGE2_PATH); return 1; }
        done += r;
    }
    close(fd);

    // ---- Open and check the target ----
    struct stat dst;
    if (stat(cfg.dev, &dst) != 0) { perror(cfg.dev); return 1; }

    int is_blk = S_ISBLK(dst.st_mode);
    if (!is_blk && !S_ISREG(dst.st_mode)) {
        fprintf(stderr, "%s is neither a block device nor a regular file\n", cfg.dev);
        return 1;
    }

    // O_EXCL on a block device makes the kernel refuse it with EBUSY if it,
    // or any of its partitions, is mounted or otherwise in use.
    fd = open(cfg.dev, O_RDWR | O_CLOEXEC | (is_blk ? O_EXCL : 0));
    if (fd < 0) {
        if (errno == EBUSY)
            fprintf(stderr, "%s is in use (mounted?); unmount it first\n", cfg.dev);
        else
            perror(cfg.dev);
        return 1;
    }

    uint64_t dev_bytes;
    if (is_blk) {
        if (ioctl(fd, BLKGETSIZE64, &dev_bytes) != 0) { perror("BLKGETSIZE64"); close(fd); return 1; }
    } else {
        dev_bytes = (uint64_t)dst.st_size;
    }

    uint64_t needed = (uint64_t)(ESP_LBA + FAT32_MIN_SECTORS) * SECTOR_SIZE;
    if (dev_bytes < needed) {
        fprintf(stderr, "%s is too small: %llu bytes, need at least %llu\n",
                cfg.dev, (unsigned long long)dev_bytes, (unsigned long long)needed);
        close(fd);
        return 1;
    }

    uint64_t esp_sectors = dev_bytes / SECTOR_SIZE - ESP_LBA;
    if (esp_sectors > UINT32_MAX - ESP_LBA) esp_sectors = UINT32_MAX - ESP_LBA;

    if (!cfg.yes && !confirm(cfg.dev, dev_bytes)) {
        fprintf(stderr, "Aborted, nothing was written.\n");
        close(fd);
        return 1;
    }

    // ---- Build the MBR ----
    uint32_t esp_volume_id;
    if (getrandom(&m.disk_signature, sizeof(uint32_t), 0) != sizeof(uint32_t) ||
        getrandom(&esp_volume_id, sizeof(uint32_t), 0) != sizeof(uint32_t)) {
        perror("getrandom");
        close(fd);
        return 1;
    }

    m.reserved          = 0;
    memset(m.entry, 0, sizeof(m.entry));
    m.entry[0].status   = 0x80;            // active: some BIOSes require it
    m.entry[0].type     = MBR_NONFS;
    m.entry[0].lba      = STAGE2_LBA;
    m.entry[0].size     = (uint32_t)s2_sectors;
    m.entry[1].type     = MBR_ESP;
    m.entry[1].lba      = ESP_LBA;
    m.entry[1].size     = (uint32_t)esp_sectors;
    m.signature         = 0xAA55;

    patch p = { .lba = STAGE2_LBA, .sec = (uint16_t)s2_sectors };
    memcpy(&m.code[MBR_PATCH_OFF], &p, sizeof(patch));

    // ---- Write: stage 2 and the ESP first, then the MBR that points at them ----
    // If anything fails midway, the old MBR is still intact.
    if (write_sectors(fd, STAGE2_LBA, s2, (uint32_t)s2_sectors) != 0) {
        perror("writing stage 2");
        close(fd);
        return 1;
    }
    if (fat32_format(fd, ESP_LBA, (uint32_t)esp_sectors, esp_volume_id) != 0) {
        perror("formatting ESP");
        close(fd);
        return 1;
    }
    if (fsync(fd) != 0) { perror("fsync"); close(fd); return 1; }

    if (write_sectors(fd, 0, &m, 1) != 0) {
        perror("writing MBR");
        close(fd);
        return 1;
    }
    if (fsync(fd) != 0) { perror("fsync"); close(fd); return 1; }

    // Read back and verify
    mbr check;
    uint8_t *s2_check = malloc((size_t)s2_sectors * SECTOR_SIZE);
    heap_alloc_fail(s2_check);
    if (read_sectors(fd, 0, &check, 1) != 0 ||
        read_sectors(fd, STAGE2_LBA, s2_check, (uint32_t)s2_sectors) != 0 ||
        memcmp(&check, &m, sizeof(mbr)) != 0 ||
        memcmp(s2_check, s2, (size_t)s2_sectors * SECTOR_SIZE) != 0) {
        fprintf(stderr, "verification failed: data read back from %s does not match\n", cfg.dev);
        close(fd);
        return 1;
    }

    if (close(fd) != 0) { perror("close"); return 1; }

    // Ask the kernel to re-read the new partition table (best effort)
    if (is_blk) {
        fd = open(cfg.dev, O_RDONLY | O_CLOEXEC);
        if (fd >= 0) { ioctl(fd, BLKRRPART); close(fd); }
    }

    free(s2);
    free(s2_check);

    printf("Installed to %s: stage 2 = %lld sectors at LBA %d, "
           "ESP (FAT32) = %.1f MiB at LBA %d\n",
           cfg.dev, (long long)s2_sectors, STAGE2_LBA,
           (double)esp_sectors * SECTOR_SIZE / (1024.0 * 1024.0), ESP_LBA);
    return 0;
}
