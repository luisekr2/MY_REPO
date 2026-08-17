	.section .rodata.spec_input,"a",@progbits
	.globl sjeng_input_start
sjeng_input_start:
	.incbin "test-mini.txt"
	.globl sjeng_input_end
sjeng_input_end:
	.byte 0
