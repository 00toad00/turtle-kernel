#include <stdint.h>

//  libk
#include <string.h>
#include <stdlib.h>

//  Bios
#include <bios/bios_call.h>
#include <bios/halt.h>

//  Boot
#include <boot/bootinfo.h>
#include <boot/codes.h>

//  MM
#include <mm/e820.h>
#include <mm/alloc.h>

//  ACPI
#include <acpi/rsdp.h>
#include <acpi/xsdp.h>

//  Driver
#include <driver/header/driver.h>
#include <driver/header/blk_device.h>
#include <driver/header/fb_device.h>
#include <driver/header/con_device.h>

void main(uint32_t bootdisk) {
    bootinfo bi;
    uint64_t status = 0;

    bi.firmware = 0; // BIOS = 0; UEFI = 1;

    //      Memory Management       //

    //  E820 Memory Map
    e820_entry entry[128];
    int entries = get_e820(128, entry);

    //  Heap initialization
    map.e820 = entry;
    map.entries = entries;
    map.page_size = 4096;
    map.min = (uint32_t)&__boot_end;
    alloc_init(&map, entry);
    malloc_init(2048);

    //      Graphics initialization     //

    //  Framebuffer device/driver init
    fb_device *fb_dev = (fb_device*)calloc(1, sizeof(fb_device));
    fb_dev->dev.type = DEV_FB;
    fb_dev->dev.parent      = NULL; 
    fb_dev->dev.children    = NULL;
    fb_dev->dev.sibling     = NULL;

    driver *fb_drv = driver_attach((device*)fb_dev, FBDV_FBDEVICE | FBDV_VGA);
    if (!fb_drv) { halt(); }

    //   Console device/driver init

    con_device *con_dev = (con_device*)calloc(1, sizeof(con_device));
    con_dev->dev.type = DEV_CON;
    con_dev->dev.parent      = (device*)fb_dev;
    con_dev->dev.children    = NULL;
    con_dev->dev.sibling     = NULL;

    driver *con_drv = driver_attach((device*)con_dev, CONDV_CONDEVICE);
    if (!con_drv) { halt(); }
    else {
        con_dev->printf(con_dev, "Device [%s] Type [%s] initialized\n", fb_dev->dev.name, dev_type_name(fb_dev->dev.type));
        con_dev->printf(con_dev, "Device [%s] Type [%s] initialized\n", con_dev->dev.name, dev_type_name(con_dev->dev.type));
    }

    //  Disk device/driver init

    blk_device *blk_dev = (blk_device*)calloc(1, sizeof(blk_device));
    blk_dev->dev.type = DEV_BLK;
    blk_dev->disk_id = bootdisk;
    blk_dev->dev.parent     = NULL;
    blk_dev->dev.children   = NULL;
    blk_dev->dev.sibling    = NULL;

    driver *blk_drv = driver_attach((device*)blk_dev, BLKDV_BLKDEVICE | BLKDV_READ);
    if (!blk_drv) { 
        con_dev->printf(con_dev, "No capable disk driver located!\n Halting execution"); 
        halt(); 
    } else {
        con_dev->printf(con_dev, "Device [%s] Type [%s] initialized\n", blk_dev->dev.name, dev_type_name(blk_dev->dev.type));
    }

    //  ACPI
    rsdp *acpi_ptr = find_rsdp();
    if (!acpi_ptr) { status |= ERRCODE_RSDP_NOT_FOUND; }
    bi.rsdp = (uint64_t)(uintptr_t)acpi_ptr;

    halt();
}
