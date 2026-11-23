#include <fs/utf16.h>

int utf16_to_ascii(const uint16_t *src, char *dst, int max) {
    int written = 0;
    for (int i = 0; src[i] != 0 && i < (max-1); i++) {
        if (!(src[i] & 0xFF00)) {
            dst[i] = (char)(src[i] & 0xFF);
        } else {
            dst[i] = '?';
        }

        written++;
    }
    dst[written] = 0;
    return written;
}