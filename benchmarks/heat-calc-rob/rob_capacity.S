/*
 * ROB-capacity stress microbenchmark.
 *
 * A cold load blocks retirement at the head of the ROB.  The following
 * destination-free integer instructions can fill the ROB without consuming
 * physical registers or LSQ entries.  ROB compression should therefore allow
 * substantially more useful work to sit behind the miss.
 */
#ifndef ROB_FILL_OPS
#define ROB_FILL_OPS 384
#endif

#ifndef ROB_FILL_ROUNDS
#define ROB_FILL_ROUNDS 128
#endif

	.section .text
	.align 6
	.globl benchmark_main
	.type benchmark_main, @function
benchmark_main:
	la	t0, rob_capacity_data
	li	t1, ROB_FILL_ROUNDS
	li	t2, 0
	li	t4, 4096

	.option push
	.option norvc
1:
	ld	t3, 0(t0)
	.rept ROB_FILL_OPS
	addi	x0, x0, 1
	.endr
	add	t2, t2, t3
	add	t0, t0, t4
	addi	t1, t1, -1
	bnez	t1, 1b
	.option pop

	snez	a0, t2
	ret
	.size benchmark_main, .-benchmark_main

	.section .bss
	.balign 4096
rob_capacity_data:
	.zero ROB_FILL_ROUNDS * 4096
