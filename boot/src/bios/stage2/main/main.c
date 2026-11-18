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

//  Video
#include <video/vesa.h>
#include <video/framebuffer.h>
#include <video/bootlogo.h>
#include <video/console.h>

//  MM 
#include <mm/e820.h>
#include <mm/alloc.h>

//  Disk
#include <fs/disk.h>
#include <fs/esp.h>
#include <fs/fat32.h>

//  ACPI
#include <acpi/rsdp.h>
#include <acpi/xsdp.h>

void main(uint32_t bootdisk) {
    bdisk = bootdisk;

    bootinfo bi;
    uint64_t status = 0;

    bi.firmware = 0; // BIOS = 0; UEFI = 1;

    //  Graphics initialization
    vbe_info_block vbe_info;
    vbe_mode_info vbe_mode;
    const char *sig = "VBE2";
    memcpy(vbe_info.signature, sig, 4);
    get_vbe_controller_info(&vbe_info);

    edid_raw edid_rw;
    edid_info edid_in;
    vbe_read_edid(&edid_rw, 0);
    edid_parse(&edid_rw, &edid_in);
    uint16_t vbec = vbe_set_mode(edid_in.width, edid_in.height, 32, &vbe_info, &vbe_mode);
    if (vbec != 0x004F) {
        const char *novbe = "Failed to set video mode. Halting execution";
        char *vga = (char*)0xB8000;
        for (int i = 0; novbe[i] != 0; i++) { vga[2*i] = novbe[i]; vga[2*i+1] = 0x0F; }
        halt();
    }

    framebuffer fb;
    fb.lfb = (void*)vbe_mode.framebuffer;
    fb.width = (int)vbe_mode.width;
    fb.height = (int)vbe_mode.height;
    fb.bpp = vbe_mode.bpp;
    fb.pitch = (vbe_info.version >= 0x0300) ? vbe_mode.lin_bytes_per_scan_line : vbe_mode.bytes_per_scan_line;

    bootlogo_draw(&fb, bootlogo, 0xFFFFFFFF, 0x00000000);

    bi.video.lfb    = (uint64_t)(uintptr_t)fb.lfb;
    bi.video.width  = (uint32_t)fb.width;
    bi.video.height = (uint32_t)fb.height;
    bi.video.pitch  = (uint32_t)fb.pitch;
    bi.video.bpp    = (uint8_t)fb.bpp;

    con.fb = &fb;
    con.border.x = 10; con.border.y = 10;
    con.pos.x = 0; con.pos.y = 0;
    con.glyph.x = 8; con.glyph.y = 13;
    con.pad.x = 1; con.pad.y = 1;

    //  ACPI
    rsdp *acpi_ptr = find_rsdp();
    if (!acpi_ptr) { status |= ERRCODE_RSDP_NOT_FOUND; }
    bi.rsdp = (uint64_t)(uintptr_t)acpi_ptr;

    //  MM
    e820_entry entry[128];
    int entries = get_e820(128, entry);

    map.e820 = entry;
    map.entries = entries;
    map.page_size = 512;
    map.min = (uint32_t)&__boot_end;
    alloc_init(&map, entry);
    
    //  Kernel location
    uint64_t esp = find_esp();
    fat32_boot_sector *bpb = (fat32_boot_sector*)alloc_page(1);
    read(bpb, esp, 1);

    uint8_t *b = (uint8_t*)bpb;
    for (int i = 0; i < 512; i++) {
        printf("%x ", b[i]);
    }

    // Testing
    printf("Test\n");

    //  Handoff -> long mode -> kernel
    bi.status = status;
    halt();
}