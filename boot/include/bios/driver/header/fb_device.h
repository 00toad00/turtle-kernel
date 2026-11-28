#pragma once

#include <stdint.h>
#include <driver/header/driver.h>

typedef enum {
    FBDV_FBDEVICE   = (1 << 0),
    FBDV_VGA        = (1 << 1),
    FBDV_VESA       = (1 << 2),
} fb_dev_caps;

typedef struct fb_device {
    device      dev;

    void        *lfb;
    uint32_t    width;
    uint32_t    height;
    uint32_t    pitch;
    uint8_t     bpp;
} fb_device;