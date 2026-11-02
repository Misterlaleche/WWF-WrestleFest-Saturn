    .section .vectors,"a"
    .long __stack_top
    .long wf_m68k_reset
    .rept 254
    .long wf_m68k_fault
    .endr
    .section .text.startup,"ax"
    .global wf_m68k_reset
wf_m68k_reset:
    move.w #0x2700,%sr
    lea __bss_start,%a0
    lea __bss_end,%a1
1:  cmpa.l %a1,%a0
    bcc 2f
    clr.w (%a0)+
    bra 1b
2:  lea 0x3000,%a0
    lea 0xb000,%a1
3:  cmpa.l %a1,%a0
    bcc 4f
    clr.w (%a0)+
    bra 3b
4:  jsr wf_sfx_main
wf_m68k_fault:
    move.w #3,0xc046
5:  bra 5b
