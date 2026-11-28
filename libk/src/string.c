#include <stddef.h>
#include <stdint.h>

void *memset(void *dst, int c, size_t n) {
    for (size_t i = 0; i < n; i++) {
        ((char*)dst)[i] = (char)c;
    }
    return dst;
}

void *memcpy(void *__restrict dst, const void *__restrict src, size_t n) {
    for (size_t i = 0; i < n; i++) {
        ((char*)dst)[i] = ((char*)src)[i];
    }
    return dst;
}

int memcmp(const void *ptr1, const void *ptr2, size_t n) {
    for (size_t i = 0; i < n; i++) {
        if (((uint8_t*)ptr1)[i] < ((uint8_t*)ptr2)[i]) { return -1; }
        if (((uint8_t*)ptr1)[i] > ((uint8_t*)ptr2)[i]) { return 1; }
    }
    return 0;
}

void *memmove(void *dst, const void *src, size_t n) {
    uint8_t *d = (uint8_t*)dst;
    const uint8_t *s = (const uint8_t*)src;

    if (d == s || n == 0) { return dst; }

    if (d < s) {
        for (size_t i = 0; i < n; i++) { d[i] = s[i]; }
    } else {
        for (size_t i = n; i > 0; i--) { d[i-1] = s[i-1]; }
    }
    return dst;
}