#include <bios/bios_call.h>
#include <fs/disk.h>
#include <string.h>

#define READBUFFER_SECTORS 8
static uint8_t readbuffer[READBUFFER_SECTORS * 512] __attribute__((aligned(4096)));
uint32_t bdisk;
static disk_address_packet dap;

int read(void *buf, uint64_t lba, uint32_t n) {
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
            r.edx = bdisk;
            r.ds  = (uint32_t)&dap >> 4;
            r.esi = (uint32_t)&dap & 0xF;
            bios_call(0x13, &r);
            if (r.eflags & 1) { errcount++; }
            else { break; }
        }
        if (errcount == 3) { return -1; }

        memcpy(ptr, readbuffer, now*512);
        ptr += now*512;
        left -= now;
        lba += now;
    }

    return 0;
}