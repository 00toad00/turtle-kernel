ENTRY(_start)
OUTPUT_FORMAT(binary)

SECTIONS {
    . = 0x8000;
    .magic  : { LONG(0xB00710AD) }
    .entry  : { KEEP(*(.text.entry)) }
    .text   : { *(.text .text.*) }
    .data   : { *(.data .data.*) }
    .rodata : { *(.rodata .rodata.*) }

    .bss    : {
        __bss_start = .;
        *(.bss .bss.*);
        *(COMMON)
        __bss_end = .;
    }

    .stack (NOLOAD) : {
        . = ALIGN(16);
        __stack_bottom = .;
        . += 0x10000;           /* 64 KiB */
        __stack_top = .;
    }
    ASSERT(__stack_top <= 0x80000, "stage 2 + stack too large")

    . = ALIGN(4096);
    __boot_end = .;

    ASSERT(_start == 0x8004, "Stage-2 _start must start at 0x8004")

    /DISCARD/ : { *(.note*) *(.comment) *(.eh_frame*) }
}