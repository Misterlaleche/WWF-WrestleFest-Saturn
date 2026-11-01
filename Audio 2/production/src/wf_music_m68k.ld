OUTPUT_FORMAT("elf32-m68k")
OUTPUT_ARCH(m68k)
ENTRY(wf_m68k_reset)
PHDRS {low PT_LOAD FLAGS(5); workspace PT_LOAD FLAGS(7); wave PT_LOAD FLAGS(4);}
SECTIONS {
 .vectors 0 : {KEEP(*(.vectors))} :low
 .text 0x400 : {*(.text.startup) *(EXCLUDE_FILE (*wf_scsp_ym.o) .text) *(EXCLUDE_FILE (*wf_scsp_ym.o) .text.*)} :low
 .rodata : {*(EXCLUDE_FILE (*wf_scsp_ym.o) .rodata) *(EXCLUDE_FILE (*wf_scsp_ym.o) .rodata.*) . = ALIGN(2);} :low
 ASSERT(. <= 0x3000, "music low code overlaps PCM rings")
 .high 0xc100 : { *wf_scsp_ym.o(.text .text.* .wf_hotcode .rodata .rodata.*) *(.wf_fm_tables) *(.wf_audio_high) . = ALIGN(4); } :workspace
 .bss (NOLOAD) : {__bss_start = .; *(.bss .bss.* COMMON) . = ALIGN(4); __bss_end = .;} :workspace
 ASSERT(. <= 0x10000, "music code/state overlaps FM wave")
 .wave 0x10000 : {KEEP(*(.wf_native_wave))} :wave
 ASSERT(SIZEOF(.wave)==9216,"music wave size")
 ASSERT(SIZEOF(.vectors)==1024,"music vectors")
 __stack_bottom = 0x12800; __stack_top = 0x14000;
 /DISCARD/ : {*(.comment) *(.note*) *(.eh_frame*)}
}
