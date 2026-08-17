ROOT := /home/joe/work/specint2006-investigation
TOOLROOT := $(ROOT)/toolchain/root/usr
GCC := $(TOOLROOT)/bin/riscv64-unknown-elf-gcc
OBJCOPY := $(TOOLROOT)/bin/riscv64-unknown-elf-objcopy
SPECS := $(TOOLROOT)/lib/picolibc/riscv64-unknown-elf/picolibc.specs
PICOINC := $(TOOLROOT)/lib/picolibc/riscv64-unknown-elf/include
PICOLIB := $(TOOLROOT)/lib/picolibc/riscv64-unknown-elf/lib/rv64iafd/lp64d
EXPECTED_CHECKSUM ?= 0

SRC = attacks.c book.c crazy.c draw.c ecache.c epd.c eval.c leval.c moves.c \
	neval.c partner.c proof.c rcfile.c search.c see.c seval.c sjeng.c ttable.c utils.c
OBJECTS = $(addprefix build/,$(SRC:.c=.o))
COMMON = -march=rv64gc -mabi=lp64d -mcmodel=medany -O3 -g \
	-fno-pic -mno-relax -ffunction-sections -fdata-sections \
	-isystem $(PICOINC)

.PHONY: all clean
all: sjeng-mini.bin

build:
	mkdir -p $@

build/%.o: vendor/%.c | build
	$(GCC) $(COMMON) -Dfopen=spec_fopen -Dmain=spec_main -I. -c $< -o $@

sjeng-mini.elf: start.S adapter.c runtime.c vfs.c input.S $(OBJECTS) linker.ld
	$(GCC) $(COMMON) -nostdlib -T linker.ld -L$(PICOLIB) \
		-Wl,--defsym=__heap_bytes=0x00100000 -Wl,--build-id=none \
		-DEXPECTED_CHECKSUM=$(EXPECTED_CHECKSUM) -I. \
		start.S adapter.c runtime.c vfs.c input.S $(OBJECTS) \
		-Wl,--start-group -lm -lc -lgcc -Wl,--end-group -o $@

sjeng-mini.bin: sjeng-mini.elf
	$(OBJCOPY) -O binary $< $@

clean:
	rm -rf build sjeng-mini.elf sjeng-mini.bin
