CLANG ?= clang
LLVM_OBJCOPY ?= llvm-objcopy-21
LLD ?= /home/joe/work/01_GEN2_COMP/tools/lld-21/usr/lib/llvm-21/bin/ld.lld
SPEC_ROOT ?= /home/joe/work/specint2006-investigation/spec-int-2006
INPUT ?= $(SPEC_ROOT)/benchspec/CPU2006/429.mcf/data/test/input/inp.in
EXPECTED_CHECKSUM ?= 2997477
HEAP_BYTES ?= 268435456

SRC = mcf.c mcfutil.c readmin.c implicit.c pstart.c output.c treeup.c \
	pbla.c pflowup.c psimplex.c pbeampp.c
MCF_SRC = $(addprefix vendor/mcf/,$(SRC))
FLAGS = --target=riscv64-unknown-elf --ld-path=$(LLD) \
	-march=rv64gc -mabi=lp64d -mcmodel=medany -O3 -g \
	-ffreestanding -fno-builtin -fno-pic -mno-relax -nostdlib \
	-DWANT_STDC_PROTO -Dmain=spec_main -DEXPECTED_CHECKSUM=$(EXPECTED_CHECKSUM) \
	-DHEAP_BYTES=$(HEAP_BYTES) \
	-Ishim -Ivendor/mcf \
	-Wl,-T,linker.ld -Wl,--build-id=none

.PHONY: all clean
all: mcf-test.bin

input.o: input.S $(INPUT)
	$(CLANG) --target=riscv64-unknown-elf -march=rv64gc -mabi=lp64d -c \
		-DINPUT_FILE='"$(INPUT)"' $< -o $@

mcf-test.elf: start.S adapter.c runtime.c input.o $(MCF_SRC) linker.ld
	$(CLANG) $(FLAGS) start.S adapter.c runtime.c input.o $(MCF_SRC) -o $@

mcf-test.bin: mcf-test.elf
	$(LLVM_OBJCOPY) -O binary $< $@

clean:
	rm -f input.o mcf-test.elf mcf-test.bin
