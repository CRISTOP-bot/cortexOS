.section .text.boot
.global _start
.type _start, @function
_start:
    la sp, stack_top
    call riscv64_early_main
1:
    wfi
    j 1b
.section .bss
.align 12
stack:
    .skip 16384
stack_top:
