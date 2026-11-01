.section .text.wf_music_module_run,"ax"
.align 2
.global _wf_music_module_run
.type _wf_music_module_run,@function
_wf_music_module_run:
 mov.l .L_cache_ccr,r0
 mov #0x11,r1
 mov.b r1,@r0
 mov.l .L_cached_body,r0
 jmp @r0
 nop
.align 2
.L_cache_ccr:
 .long 0xfffffe92
.L_cached_body:
 .long _wf_music_module_run_body
.size _wf_music_module_run,.-_wf_music_module_run
