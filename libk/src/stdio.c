#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

int sprintf(char *buf, const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    char num[66];
    char *out = buf;

    for (; *fmt; fmt++) {
        if (*fmt != '%') { *out++ = *fmt; continue; }

        int wide = 0;
        while (fmt[1] == 'l') { wide = 1; fmt++; }

        switch (*++fmt) {
        case 's': {
            const char *s = va_arg(ap, const char *);
            if (!s) s = "(null)";
            while (*s) *out++ = *s++;
            break;
        }
        case 'u':
        case 'x':
        case 'o': {
            int base = (*fmt == 'u') ? 10 : (*fmt == 'x') ? 16 : 8;
            uint64_t v = wide ? va_arg(ap, uint64_t) : va_arg(ap, uint32_t);
            utoa(v, base, num);
            for (char *p = num; *p; p++) *out++ = *p;
            break;
        }
        case 'd':
        case 'i': {
            int64_t v = wide ? va_arg(ap, int64_t) : va_arg(ap, int32_t);
            itoa(v, num);
            for (char *p = num; *p; p++) *out++ = *p;
            break;
        }
        case 'c': *out++ = (char)va_arg(ap, int); break;
        case '%': *out++ = '%'; break;
        case '\0': fmt--; break;
        default:  break;
        }
    }
    *out = '\0';
    va_end(ap);
    return (int)(out - buf);
}

int snprintf(char *buf, size_t n, const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    char num[66];
    size_t pos = 0;

    #define PUT(c) do { if (pos + 1 < n) buf[pos] = (c); pos++; } while (0)

    for (; *fmt; fmt++) {
        if (*fmt != '%') { PUT(*fmt); continue; }

        int wide = 0;
        while (fmt[1] == 'l') { wide = 1; fmt++; }

        switch (*++fmt) {
        case 's': {
            const char *s = va_arg(ap, const char *);
            if (!s) s = "(null)";
            while (*s) PUT(*s++);
            break;
        }
        case 'u':
        case 'x':
        case 'o': {
            int base = (*fmt == 'u') ? 10 : (*fmt == 'x') ? 16 : 8;
            uint64_t v = wide ? va_arg(ap, uint64_t) : va_arg(ap, uint32_t);
            utoa(v, base, num);
            for (char *p = num; *p; p++) PUT(*p);
            break;
        }
        case 'd':
        case 'i': {
            int64_t v = wide ? va_arg(ap, int64_t) : va_arg(ap, int32_t);
            itoa(v, num);
            for (char *p = num; *p; p++) PUT(*p);
            break;
        }
        case 'c': PUT((char)va_arg(ap, int)); break;
        case '%': PUT('%'); break;
        case '\0': fmt--; break;
        default:  break;
        }
    }
    #undef PUT

    if (n > 0) buf[pos < n ? pos : n - 1] = '\0';
    va_end(ap);
    return (int)pos;
}