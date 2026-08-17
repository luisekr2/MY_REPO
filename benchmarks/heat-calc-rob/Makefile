CLANG ?= clang
LLVM_OBJCOPY ?= llvm-objcopy-21
LLD ?= ../../tools/lld-21/usr/lib/llvm-21/bin/ld.lld
STEPS ?= 32
ROB_FILL_OPS ?= 384
ROB_FILL_ROUNDS ?= 128

COMMON_FLAGS = \
	--target=riscv64-unknown-elf --ld-path=$(LLD) \
	-march=rv64gc -mabi=lp64d -mcmodel=medany \
	-O3 -g -ffreestanding -fno-builtin \
	-fno-vectorize -fno-slp-vectorize -ffp-contract=off \
	-fno-pic -mno-relax -nostdlib \
	-Wl,-T,linker.ld -Wl,--build-id=none -DSTEPS=$(STEPS)

.PHONY: all clean

all: heat-calc-native.bin heat-calc-tage-stress.bin rob-capacity.bin

heat-calc-native.elf: start.S heat_calc.c linker.ld
	$(CLANG) $(COMMON_FLAGS) start.S heat_calc.c -o $@

heat-calc-tage-stress.elf: start.S heat_calc.c linker.ld
	$(CLANG) $(COMMON_FLAGS) -DTAGE_STRESS start.S heat_calc.c -o $@

rob-capacity.elf: start.S rob_capacity.S linker.ld
	$(CLANG) $(COMMON_FLAGS) \
		-DROB_FILL_OPS=$(ROB_FILL_OPS) \
		-DROB_FILL_ROUNDS=$(ROB_FILL_ROUNDS) \
		start.S rob_capacity.S -o $@

%.bin: %.elf
	$(LLVM_OBJCOPY) -O binary $< $@

clean:
	rm -f heat-calc-native.elf heat-calc-native.bin
	rm -f heat-calc-tage-stress.elf heat-calc-tage-stress.bin
	rm -f rob-capacity.elf rob-capacity.bin
