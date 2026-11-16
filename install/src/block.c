#include <stdint.h>
#include <errno.h>
#include <unistd.h>
#include <sys/stat.h>

#include <block.h>

int read_sectors(int fd, uint64_t lba, void *buf, uint32_t count) {
    uint8_t *p   = buf;
    size_t  left = (size_t)count * SECTOR_SIZE;
    off_t   off  = (off_t)(lba * SECTOR_SIZE);

    while (left > 0) {
        ssize_t r = pread(fd, p, left, off);
        if (r < 0) {
            if (errno == EINTR) continue;
            return -1;
        }
        if (r == 0) { errno = EIO; return -1; }    // unexpected EOF
        p += r; off += r; left -= r;
    }
    return 0;
}

int write_sectors(int fd, uint64_t lba, const void *buf, uint32_t count) {
    const uint8_t *p = buf;
    size_t  left = (size_t)count * SECTOR_SIZE;
    off_t   off  = (off_t)(lba * SECTOR_SIZE);

    while (left > 0) {
        ssize_t w = pwrite(fd, p, left, off);
        if (w < 0) {
            if (errno == EINTR) continue;
            return -1;
        }
        if (w == 0) { errno = ENOSPC; return -1; }  // end of device
        p += w; off += w; left -= w;
    }
    return 0;
}

// Size of a file in sectors (rounded up), or -1 with errno set on error.
int64_t file_sectors(const char *path) {
    struct stat st;
    if (stat(path, &st) != 0) return -1;
    return ((int64_t)st.st_size + SECTOR_SIZE - 1) / SECTOR_SIZE;
}
