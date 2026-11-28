#pragma once

#include <stdint.h>
#include <driver/header/driver.h>

typedef enum {
    CONDV_CONDEVICE = (1 << 0),
} con_dev_caps;

typedef struct {
    int x;
    int y;
} dim;

typedef struct con_device {
    device      dev;

    dim pos;
    dim border;

    dim glyph;
    dim pad; 
    
    void    (*draw_char)(struct con_device *self, char c, int x, int y);
    void    (*printf)(struct con_device *self, const char *fmt, ...);
} con_device;