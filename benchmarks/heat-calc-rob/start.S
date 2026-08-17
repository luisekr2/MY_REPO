    .section .text.init
    .globl _start
    .type _start, @function
_start:
    la      sp, __stack_top
    li      t0, 0x6000
    csrs    mstatus, t0
    call    benchmark_main
    beqz    a0, 1f
    li      a0, 1
    .word   0x0000006b
0:
    j       0b
1:
    li      a0, 0
    .word   0x0000006b
    j       0b
