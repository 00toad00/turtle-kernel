#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#include <driver/header/driver.h>
#include <driver/header/con_device.h>
#include <driver/header/fb_device.h>

static driver drv;

static int devices = 0;

static void text_draw_char(con_device *self, char c, int x, int y) {
    fb_device *fb_dev = (fb_device*)self->dev.parent;

    int i = y*fb_dev->pitch + x*(fb_dev->bpp/8);
    char *vga = (char*)fb_dev->lfb;
    vga[i++] = c; vga[i] = 0x0F;
}

static void print_c(con_device *self, const char c) {
    fb_device *fb_dev = (fb_device*)self->dev.parent;
    int line_h = self->glyph.y + self->pad.y;

    if (c == '\r') { self->pos.x = 0; return; }

    if (c == '\n' || self->border.x + self->pos.x + self->glyph.x + self->pad.x
            > (int)(fb_dev->width - self->border.x)) {
        self->pos.x = 0;
        self->pos.y += line_h;

        // scroll if the new line doesn't fit
        if (self->border.y + self->pos.y + line_h > (int)(fb_dev->height - self->border.y)) {
            uint8_t *base  = (uint8_t*)fb_dev->lfb;
            uint32_t shift = line_h * fb_dev->pitch;
            uint32_t total = fb_dev->height * fb_dev->pitch;
            memmove(base, base + shift, total - shift);

            self->pos.y -= line_h;
            for (int x = 0; self->border.x + x + self->glyph.x <= (int)(fb_dev->width - self->border.x);
                 x += self->glyph.x + self->pad.x) {
                self->draw_char(self, ' ', self->border.x + x, self->border.y + self->pos.y);
            }
        }
        if (c == '\n') { return; }
    }

    self->draw_char(self, c, self->border.x + self->pos.x, self->border.y + self->pos.y);
    self->pos.x += self->glyph.x + self->pad.x;
}

static void print_s(con_device *self, const char *str) {
    while (*str) print_c(self, *str++);
}

static void printf(con_device *self, const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    char buf[66];

    for (; *fmt; fmt++) {
        if (*fmt != '%') { print_c(self, *fmt); continue; }

        int wide = 0;
        while (fmt[1] == 'l') { wide = 1; fmt++; }

        switch (*++fmt) {
        case 's': {
            const char *s = va_arg(ap, const char *);
            print_s(self, s ? s : "(null)");
            break;
        }
        case 'u':
        case 'x':
        case 'o': {
            int base = (*fmt == 'u') ? 10 : (*fmt == 'x') ? 16 : 8;
            uint64_t v = wide ? va_arg(ap, uint64_t) : va_arg(ap, uint32_t);
            utoa(v, base, buf);
            print_s(self, buf);
            break;
        }
        case 'd':
        case 'i': {
            int64_t v = wide ? va_arg(ap, int64_t) : va_arg(ap, int32_t);
            itoa(v, buf);
            print_s(self, buf);
            break;
        }
        case 'c': print_c(self, (char)va_arg(ap, int)); break;
        case '%': print_c(self, '%'); break;
        case '\0': fmt--; break;
        default:  break;
        }
    }
    va_end(ap);
}

static int probe(device *dev) {
    con_device *con_dev = (con_device*)dev;

    if (dev->type != DEV_CON) { return 0; }

    memset(&con_dev->pos, 0, sizeof(dim));
    memset(&con_dev->border, 0, sizeof(dim));
    memset(&con_dev->pad, 0, sizeof(dim));
    con_dev->glyph.x = 1; con_dev->glyph.y = 1;

    return CONDV_CONDEVICE;
}

static void register_device(device *dev) {
    con_device *con_dev = (con_device*)dev;

    con_dev->draw_char = text_draw_char;
    con_dev->printf = printf;

    snprintf(con_dev->dev.name, 32, "tty%u", devices++);
    con_dev->dev.drv = &drv;
}

static driver drv = {
    .name               = "CONSOLE",
    .probe              = probe,
    .register_device    = register_device,
};
REGISTER_DRIVER(drv);