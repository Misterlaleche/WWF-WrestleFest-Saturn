OUTPUT_FORMAT("elf32-sh")
OUTPUT_ARCH(sh)
ENTRY(_wf_music_module_prepare)
PHDRS {program PT_LOAD FLAGS(5); hot PT_LOAD FLAGS(5); state PT_LOAD FLAGS(6); shared PT_LOAD FLAGS(6);}
SECTIONS {
 .header 0x05c5c000 : {KEEP(*(.wf_music_header))} :program
 .text : {*(.text .text.*)} :program
 .rodata : {*(.rodata .rodata.*) *(.wf_fm_tables) . = ALIGN(4);} :program
 __hotcode_load = .;
 .wf_hotcode 0x60889d0 : AT(__hotcode_load) {
  _wf_hotcode_start = .; *(.wf_hotcode .wf_hotcode.*) *(.wf_hotdata_const .wf_hotdata_const.*) . = ALIGN(4); _wf_hotcode_end = .;
 } :hot
 _wf_hotcode_load = LOADADDR(.wf_hotcode);
 ASSERT(__hotcode_load + SIZEOF(.wf_hotcode) <= 0x05c7bf00,"transport exceeds scratch")
 ASSERT(SIZEOF(.wf_hotcode) + 3220 <= 18432,"native code exceeds kernel allocation")
 ASSERT(_wf_hotcode_end + 3220 <= 0x608d1d0,"state exceeds bound master hot window")
 .bss 0x05c7c000 (NOLOAD) : {*(.bss .bss.* COMMON) . = ALIGN(4);} :state
 ASSERT(. <= 0x05c7f000,"music private state exceeds12KiB")
 .shared 0x05c7f000 (NOLOAD) : {. += 4096;} :shared
 __music_image_end = 0x05c80000;
 /DISCARD/ : {*(.comment) *(.note*) *(.eh_frame*)}
}
