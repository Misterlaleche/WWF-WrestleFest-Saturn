/* Baseline 68000 block decoder. LUT values are generated from the audited
 * 49-step MSM6295 recurrence, not approximated. d.index remains 0..48. */
    .text
    .global wf_sfx_decode_block
wf_sfx_decode_block:
    movem.l %d2-%d7/%a2-%a4,-(%sp)
    movea.l 40(%sp),%a4
    movea.l 44(%sp),%a0
    adda.l (%a4),%a0
    movea.l 48(%sp),%a1
    move.l 52(%sp),%d7
    tst.l %d7
    beq 8f
    subq.w #1,%d7
    move.l 4(%a4),%d6
    move.w 8(%a4),%d4
    move.w 10(%a4),%d5
    lsl.w #6,%d5
    moveq #0,%d3
    move.b 12(%a4),%d3
    lea wf_sfx_lut,%a2
1:  tst.l %d6
    beq 6f
    moveq #0,%d0
    move.b (%a0),%d0
    tst.w %d3
    beq 2f
    andi.w #15,%d0
    addq.l #1,%a0
    bra 3f
2:  lsr.w #4,%d0
3:  eori.w #1,%d3
    subq.l #1,%d6
    lsl.w #2,%d0
    add.w %d5,%d0
    lea (%a2,%d0.w),%a3
    add.w (%a3)+,%d4
    move.w (%a3),%d5
    cmpi.w #2047,%d4
    ble 4f
    move.w #2047,%d4
4:  cmpi.w #-2048,%d4
    bge 5f
    move.w #-2048,%d4
5:  move.w %d4,%d0
    lsl.w #4,%d0
    move.w %d0,(%a1)+
    dbra %d7,1b
    bra 7f
6:  clr.w (%a1)+
    dbra %d7,6b
7:  suba.l 44(%sp),%a0
    move.l %a0,(%a4)
    move.l %d6,4(%a4)
    move.w %d4,8(%a4)
    lsr.w #6,%d5
    move.w %d5,10(%a4)
    move.b %d3,12(%a4)
8:  movem.l (%sp)+,%d2-%d7/%a2-%a4
    rts
    .section .rodata.wf_sfx_lut,"a"
    .balign 2
wf_sfx_lut:
    .include "oki-lut.inc"
