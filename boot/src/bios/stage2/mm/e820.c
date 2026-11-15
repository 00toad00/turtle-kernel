#include <bios/bios_call.h>
#include <string.h>
#include <mm/e820.h>

int get_e820(int max, e820_entry *map) {
    regs r;
    e820_entry tmp;
    uint32_t addr = (uint32_t)&tmp;
    uint32_t cont = 0;
    int count = 0;

    do {
        memset(&r, 0, sizeof(regs));
        tmp.acpi = 1;
        r.eax = 0xE820;
        r.edx = 0x534D4150;
        r.ebx = cont;
        r.ecx = sizeof(e820_entry);
        r.es = addr >> 4;
        r.edi = addr & 0xF;
        bios_call(0x15, &r);

        if (r.eflags & 1 || r.eax != 0x534D4150) {
            if (count == 0) { return -1; }
            break;
        }
        cont = r.ebx;

        if (r.ecx < 20 || tmp.length == 0) { continue; }
        if (r.ecx >= 24 && !(tmp.acpi & 1)) { continue; }

        if (count < max) {
            map[count++] = tmp;
        }

    } while (cont != 0 && count < max);

    return count;
}