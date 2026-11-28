#include <stdint.h>
#include <string.h>
#include <stdio.h>

#include <driver/header/driver.h>
#include <driver/header/blk_device.h>
#include <driver/driver/disk.h>

#include <bios/bios_call.h>

static driver drv;

static int devices = 0;

#define READBUFFER_SECTORS 8
static uint8_t readbuffer[READBUFFER_SECTORS * 512] __attribute__((aligned(4096)));
static disk_address_packet dap;
static drive_params dp;

static void *read(blk_device *self, void *buf, uint64_t lba, uint32_t n) {
    uint32_t addr = (uint32_t)&readbuffer[0];
    char *ptr = (char*)buf;

    uint32_t sec = sizeof(readbuffer)/512;

    uint32_t left = n;
    uint32_t now = 0;

    while (1) {
        if (left == 0) { break; } 
        else if (left > sec) { now = sec; }
        else { now = left; }

        int errcount = 0;
        for (int i = 0; i < 3; i++) {
            dap.size = 16;
            dap.zero = 0;
            dap.n    = now;
            dap.seg  = addr >> 4;
            dap.off  = addr & 0xF;
            dap.lba  = lba;
        
            regs r; memset(&r, 0, sizeof(regs));
            r.eax = 0x4200;
            r.edx = (uint32_t)self->disk_id;
            r.ds  = (uint32_t)&dap >> 4;
            r.esi = (uint32_t)&dap & 0xF;
            bios_call(0x13, &r);
            if (r.eflags & 1) { errcount++; }
            else { break; }
        }
        if (errcount == 3) { return NULL; }

        memcpy(ptr, readbuffer, now*512);
        ptr += now*512;
        left -= now;
        lba += now;
    }

    return buf;
}

static int probe(device *dev) {
    blk_device *blk_dev = (blk_device*)dev;

    if (dev->type != DEV_BLK) { return 0; }
    
    regs r; memset(&r, 0, sizeof(regs));
    r.eax = 0x4100;
    r.ebx = 0x55AA;
    r.edx = blk_dev->disk_id;
    bios_call(0x13, &r);

    if (r.eflags & 1) { return 0; }
    if ((r.ebx & 0xFFFF) != 0xAA55) { return 0; }
    if (!(r.ecx & 1)) { return 0; }

    memset(&dp, 0, sizeof(dp));
    dp.size = sizeof(dp);
    memset(&r, 0, sizeof(regs));
    r.eax   = 0x4800;
    r.edx   = blk_dev->disk_id;
    r.ds    = (uint32_t)&dp >> 4;
    r.esi   = (uint32_t)&dp & 0xF;
    bios_call(0x13, &r);

    if (r.eflags & 1) { return 0; }
    if (dp.sector_size == 0) { return 0; }

    blk_dev->sectors = dp.sectors;
    blk_dev->bps = dp.sector_size;

    blk_dev->dev.capabilities = BLKDV_BLKDEVICE | BLKDV_READ;
    return blk_dev->dev.capabilities;
}

static void register_device(device *dev) {
    blk_device *blk_dev = (blk_device*)dev;

    blk_dev->read = read;

    snprintf(blk_dev->dev.name, 32, "disk%u", devices++);
    blk_dev->dev.drv = &drv;
}

static driver drv = { 
    .name               = "BIOS DISK DRIVER",
    .probe              = probe,
    .register_device    = register_device
};
REGISTER_DRIVER(drv);