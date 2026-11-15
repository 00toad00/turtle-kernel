.section .text.entry

.global _start
.extern main
.extern memset

.extern __bss_start
.extern __bss_end

.code32

_start:
    pushl   %ebp
    movl    %esp, %ebp

    cld
    
    movl    $__bss_end, %eax
    subl    $__bss_start, %eax

    pushl   %eax
    pushl   $0
    pushl   $__bss_start
    call    memset
    addl    $12, %esp

    movl    8(%ebp), %eax
    pushl   %eax
    call    main

    movl    %ebp, %esp
    popl    %ebp
    ret
    