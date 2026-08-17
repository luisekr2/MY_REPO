	.section .rodata.spec_input,"a",@progbits
	.globl spec_input_start
spec_input_start:
	.incbin INPUT_FILE
	.globl spec_input_end
spec_input_end:
	.byte 0
