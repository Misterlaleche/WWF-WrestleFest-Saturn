OUTPUT_FORMAT("elf32-m68k")
OUTPUT_ARCH(m68k)
ENTRY(wf_m68k_reset)
PHDRS {
  low PT_LOAD FLAGS(7);
  workspace PT_LOAD FLAGS(6);
  wave PT_LOAD FLAGS(4);
}
SECTIONS {
  .vectors 0 : { KEEP(*(.vectors)) } :low
  .text 0x400 : { *(.text.startup) *(.text .text.*) } :low
  .rodata : { *(.rodata) *(.rodata.*) } :low
  .data : { *(.data .data.*) } :low
  .bss (NOLOAD) : {
    . = ALIGN(4); __bss_start = .;
    *(.bss .bss.* COMMON)
    . = ALIGN(4); __bss_end = .;
  } :low
  ASSERT(. <= 0xbc00, "low driver overlaps legacy communication")
  .adx_bss 0xc100 (NOLOAD) : {
    __adx_start = .; *(.adx_bss)
    . = ALIGN(4); __adx_end = .;
  } :workspace
  ASSERT(. <= 0x12000, "ADX workspace overlaps stack")
  __stack_bottom = 0x12000;
  __stack_top = 0x14000;
  .wave 0x70000 : { *(.wf_native_wave) } :wave
  ASSERT(SIZEOF(.wave) == 6144, "wave must be exactly 6144 bytes")
  ASSERT(SIZEOF(.vectors) == 1024, "full 68000 vector table required")
  /DISCARD/ : { *(.comment) *(.note*) *(.eh_frame*) }
}
