/* Baseline68000 exact MSM6295 block decoder, paired encoded-byte path.
 * The audited LUT/recurrence, clamps and public decoder state are unchanged.
 * Valid state: index0..48, signal-2048..2047, low0/1. The historic DBRA
 * count semantics are retained (nonzero counts use their low16 bits, with
 * low16==0 meaning65536 samples). Exhausted input emits zero without reads. */
    .text
    .macro DECODE_OFFSET
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
    .endm
    .global wf_sfx_decode_block
wf_sfx_decode_block:
    movem.l %d2-%d7/%a2-%a4,-(%sp)
    movea.l 40(%sp),%a4
    movea.l 44(%sp),%a0
    adda.l (%a4),%a0
    movea.l 48(%sp),%a1
    move.l 52(%sp),%d7
    tst.l %d7
    beq .L_return
    andi.l #65535,%d7
    bne .L_count
    move.l #65536,%d7
.L_count:
    move.l 4(%a4),%d6
    moveq #0,%d3
    cmp.l %d6,%d7
    bls .L_valid
    move.l %d7,%d3
    move.l %d6,%d7
    sub.l %d7,%d3
.L_valid:
    sub.l %d7,%d6
    move.w 8(%a4),%d4
    move.w 10(%a4),%d5
    lsl.w #6,%d5
    lea wf_sfx_lut,%a2
    tst.l %d7
    beq .L_finished
    moveq #0,%d0
    move.b 12(%a4),%d0
    move.l %d7,%d2
    eor.b %d0,%d2
    andi.b #1,%d2
    move.b %d2,12(%a4)
    tst.b %d0
    beq .L_split
    /* Complete a pending low nibble once, then enter the byte-pair path. */
    moveq #0,%d0
    move.b (%a0)+,%d0
    andi.w #15,%d0
    add.w %d0,%d0
    add.w %d0,%d0
    DECODE_OFFSET
    subq.l #1,%d7
    beq .L_finished
.L_split:
    btst #0,%d7
    beq .L_even
    bset #31,%d3
.L_even:
    lsr.l #1,%d7
    beq .L_tail
    subq.w #1,%d7
.L_pair:
    /* One input read supplies both nibbles. Neither low nor remaining
     * needs a per-sample toggle/test; the whole valid span was bounded. */
    moveq #0,%d2
    move.b (%a0)+,%d2
    move.w %d2,%d0
    andi.w #240,%d0
    lsr.w #2,%d0
    DECODE_OFFSET
    move.w %d2,%d0
    andi.w #15,%d0
    add.w %d0,%d0
    add.w %d0,%d0
    DECODE_OFFSET
    dbra %d7,.L_pair
.L_tail:
    tst.l %d3
    bpl .L_finished
    bclr #31,%d3
    /* A final high nibble does not advance the encoded-byte offset. */
    moveq #0,%d0
    move.b (%a0),%d0
    andi.w #240,%d0
    lsr.w #2,%d0
    DECODE_OFFSET
.L_finished:
    tst.l %d3
    beq .L_store
    subq.w #1,%d3
.L_zero:
    clr.w (%a1)+
    dbra %d3,.L_zero
.L_store:
    suba.l 44(%sp),%a0
    move.l %a0,(%a4)
    move.l %d6,4(%a4)
    move.w %d4,8(%a4)
    lsr.w #6,%d5
    move.w %d5,10(%a4)
.L_return:
    movem.l (%sp)+,%d2-%d7/%a2-%a4
    rts
    .section .rodata.wf_sfx_lut,"a"
    .balign 2
wf_sfx_lut:
    .include "oki-lut.inc"
