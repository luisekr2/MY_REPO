	.section .text.init
	.globl _start
_start:
	la	sp, _stack_top
	csrr	t0, mstatus
	li	t1, 0x6000
	or	t0, t0, t1
	csrw	mstatus, t0
	call	benchmark_main
	bnez	a0, bad_trap
good_trap:
	li	a0, 0
	.word	0x0000006b
	j	good_trap
bad_trap:
	li	a0, 1
	.word	0x0000006b
	j	bad_trap
