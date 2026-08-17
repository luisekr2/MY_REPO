ENTRY(_start)

SECTIONS
{
  . = 0x80000000;
  .text : { *(.text.init) *(.text .text.*) }
  .rodata : { *(.rodata .rodata.*) }
  .data : { *(.data .data.*) }
  .bss : {
    __bss_start = .;
    *(.bss .bss.* COMMON)
    __bss_end = .;
  }
  . = ALIGN(64);
  __heap_start = .;
  . += DEFINED(__heap_bytes) ? __heap_bytes : 0x08000000;
  __heap_end = .;
  . = ALIGN(16);
  . += 0x20000;
  _stack_top = .;
}
