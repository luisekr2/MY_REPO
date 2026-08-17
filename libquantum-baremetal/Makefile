CLANG ?= clang
LLVM_OBJCOPY ?= llvm-objcopy-21
LLD ?= /home/joe/work/01_GEN2_COMP/tools/lld-21/usr/lib/llvm-21/bin/ld.lld

SRC = classic.c complex.c decoherence.c expn.c gates.c matrix.c measure.c \
	oaddn.c objcode.c omuln.c qec.c qft.c qureg.c shor.c version.c specrand.c
VENDOR_SRC = $(addprefix vendor/,$(SRC))
FLAGS = --target=riscv64-unknown-elf --ld-path=$(LLD) \
	-march=rv64gc -mabi=lp64d -mcmodel=medany -O3 -g \
	-ffreestanding -fno-builtin -fno-math-errno -fno-pic -mno-relax -nostdlib \
	-DSPEC_CPU -DSPEC_CPU_LINUX -Dmain=spec_main -Ishim -Ivendor \
	-DLQ_WIDTH=$(LQ_WIDTH) -DLQ_ROUNDS=$(LQ_ROUNDS) \
	-Wl,-T,linker.ld -Wl,--build-id=none

LQ_WIDTH ?= 8
LQ_ROUNDS ?= 1

.PHONY: all clean
all: libquantum-test.bin

libquantum-test.elf: start.S adapter.c runtime.c $(VENDOR_SRC) linker.ld
	$(CLANG) $(FLAGS) start.S adapter.c runtime.c $(VENDOR_SRC) -o $@

libquantum-test.bin: libquantum-test.elf
	$(LLVM_OBJCOPY) -O binary $< $@

clean:
	rm -f libquantum-test.elf libquantum-test.bin
