CLANG ?= clang
LLVM_OBJCOPY ?= llvm-objcopy-21
LLD ?= ../../tools/lld-21/usr/lib/llvm-21/bin/ld.lld

BZIP2_SRC ?= vendor/bzip2-1.0.3
INPUT_BYTES ?= 256

COMMON_FLAGS = \
	--target=riscv64-unknown-elf --ld-path=$(LLD) \
	-march=rv64gc -mabi=lp64d -mcmodel=medany \
	-O3 -g -ffreestanding -fno-builtin -fno-pic -mno-relax -nostdlib \
	-DBZ_NO_STDIO -DINPUT_BYTES=$(INPUT_BYTES) \
	-Ishim -I$(BZIP2_SRC) \
	-Wl,-T,linker.ld -Wl,--build-id=none

BZIP2_C = \
	$(BZIP2_SRC)/blocksort.c \
	$(BZIP2_SRC)/bzlib.c \
	$(BZIP2_SRC)/compress.c \
	$(BZIP2_SRC)/crctable.c \
	$(BZIP2_SRC)/decompress.c \
	$(BZIP2_SRC)/huffman.c \
	$(BZIP2_SRC)/randtable.c

.PHONY: all clean

all: bzip2-derived.bin

bzip2-derived.elf: start.S benchmark.c baremetal_runtime.c linker.ld
	$(CLANG) $(COMMON_FLAGS) start.S benchmark.c baremetal_runtime.c $(BZIP2_C) -o $@

%.bin: %.elf
	$(LLVM_OBJCOPY) -O binary $< $@

clean:
	rm -f bzip2-derived.elf bzip2-derived.bin
