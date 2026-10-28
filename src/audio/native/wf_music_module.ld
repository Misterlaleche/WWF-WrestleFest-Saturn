OUTPUT_FORMAT("elf32-sh")
OUTPUT_ARCH(sh)
ENTRY(_wf_music_module_prepare)
PHDRS {program PT_LOAD FLAGS(5); state PT_LOAD FLAGS(6); shared PT_LOAD FLAGS(6);}
SECTIONS {
 .header 0x05c5c000 : {KEEP(*(.wf_music_header))} :program
 .text : {*(.text .text.*)} :program
 .rodata : {*(.rodata .rodata.*) *(.wf_fm_tables) . = ALIGN(4);} :program
 ASSERT(. <= 0x05c7bf00,"music runtime ELF exceeds128KiB scratch")
 .bss 0x05c7c000 (NOLOAD) : {*(.bss .bss.* COMMON) . = ALIGN(4);} :state
 ASSERT(. <= 0x05c7f000,"music private state exceeds12KiB")
 .shared 0x05c7f000 (NOLOAD) : {. += 4096;} :shared
 __music_image_end = 0x05c80000;
 /DISCARD/ : {*(.comment) *(.note*) *(.eh_frame*)}
}
