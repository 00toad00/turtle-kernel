.section .text.entry

.global _start
.extern main
.extern memset
.extern halt

.extern __bss_start
.extern __bss_end
.extern __stack_top

.code32

_start:
    movl    4(%esp), %ebx

    movl    $__stack_top, %esp
    xorl    %ebp, %ebp

    cld
    movl    $__bss_end, %eax
    subl    $__bss_start, %eax
    pushl   %eax
    pushl   $0
    pushl   $__bss_start
    call    memset
    addl    $12, %esp

    subl    $12, %esp
    pushl   %ebx
    call    main
    call    halt
    