#include <stddef.h>

#include <driver/header/driver.h>

driver *driver_attach(device *dev, int needed) {
    for (driver **d = __driver_start; d < __driver_end; d++) {
        int caps = (*d)->probe(dev);
        if ((caps & needed) == needed) {
            (*d)->register_device(dev);
            return *d;
        }
    }
    return NULL;    
}

const char *dev_type_name(dev_type t) {
    switch (t) {
    case DEV_BLK: return "BLOCK";
    case DEV_FB:  return "SCREEN";
    case DEV_CON: return "CONSOLE";
    default:      return "UNKNOWN";
    }
}