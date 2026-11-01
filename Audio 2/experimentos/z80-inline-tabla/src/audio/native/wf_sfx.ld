OUTPUT_FORMAT("elf32-m68k")
OUTPUT_ARCH(m68k)
ENTRY(wf_m68k_reset)
PHDRS { low PT_LOAD FLAGS(5); state PT_LOAD FLAGS(6); wave PT_LOAD FLAGS(4); }
SECTIONS {
 .vectors 0 : { KEEP(*(.vectors)) } :low
 .text 0x400 : { *(.text.startup) *(.text .text.*) } :low
 .rodata : { *(.rodata .rodata.*) . = ALIGN(2); } :low
 ASSERT(. <= 0x3000, "native SFX code overlaps PCM rings")
 .bss 0xc100 (NOLOAD) : {
  __bss_start = .; *(.bss .bss.* COMMON) . = ALIGN(4); __bss_end = .;
 } :state
 ASSERT(. <= 0x10000, "native SFX BSS overlaps FM wave")
 .wave 0x10000 : { *(.wf_native_wave) } :wave
 ASSERT(SIZEOF(.wave) == 6144, "FM wave size")
 ASSERT(SIZEOF(.vectors) == 1024, "vector size")
 __stack_bottom = 0x12000; __stack_top = 0x14000;
 /DISCARD/ : { *(.comment) *(.note*) *(.eh_frame*) }
}
