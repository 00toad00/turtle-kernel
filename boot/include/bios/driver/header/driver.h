#pragma once

#include <stdint.h>

extern struct driver *__driver_start[];
extern struct driver *__driver_end[];

struct device;
struct driver;

typedef enum {
    DEV_BLK = 1,
    DEV_FB,
    DEV_CON,
} dev_type;

typedef struct driver {
    char    name[32];
    int     (*probe)(struct device *dev);
    void    (*register_device)(struct device *dev);
} driver;

typedef struct device {
    dev_type        type;
    int             capabilities;
    char            name[32];

    struct device   *parent, *children, *sibling;
    struct device   *prev, *next;

    driver          *drv;
} device;

#define REGISTER_DRIVER(d) static driver *__drv_ptr_##d __attribute__((section(".driver"), used)) = &d

driver *driver_attach(device *dev, int needed);
const char *dev_type_name(dev_type t);