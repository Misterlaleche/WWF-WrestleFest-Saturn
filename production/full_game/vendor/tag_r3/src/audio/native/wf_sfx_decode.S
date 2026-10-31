/* WP2015 exact MSM6295 decoder: internal unsigned, biased Q4 signal.
 * (signal+2048)*16 is always a multiple of16 in0..65520. Thus carry on
 * addition and borrow on subtraction are exactly the original saturation
 * conditions. Flipping bit15 emits identical signed PCM16; the public
 * decoder state is converted back to the original12-bit signed signal.
 * Input signal/index retain the original reachable ranges[-2048,2047]/[0,48].
 */
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
    beq .Lreturn
    move.l 4(%a4),%d6
    move.w 8(%a4),%d4
    addi.w #2048,%d4
    lsl.w #4,%d4
    move.w 10(%a4),%d5
    lsl.w #6,%d5
    moveq #0,%d3
    move.b 12(%a4),%d3
    lea wf_sfx_lut,%a2
    move.l %d7,%d2
    cmp.l %d6,%d2
    bls .Lbounded
    move.l %d6,%d2
.Lbounded:
    sub.l %d2,%d7
    sub.l %d2,%d6
    tst.l %d2
    beq .Lzeros
    tst.w %d3
    beq .Lpairs_setup
    /* Finish the low nibble left by a preceding odd-size call. */
    moveq #0,%d1
    move.b (%a0)+,%d1
    move.w %d1,%d0
    andi.w #15,%d0
    lsl.w #2,%d0
    add.w %d5,%d0
    lea (%a2,%d0.w),%a3
    btst #3,%d1
    bne .Llead_minus
    add.w (%a3)+,%d4
    bcc .Llead_store
    moveq #-16,%d4
    bra .Llead_store
.Llead_minus:
    sub.w (%a3)+,%d4
    bcc .Llead_store
    moveq #0,%d4
.Llead_store:
    move.w (%a3),%d5
    move.w %d4,%d0
    eori.w #0x8000,%d0
    move.w %d0,(%a1)+
    moveq #0,%d3
    subq.l #1,%d2
    beq .Lzeros
.Lpairs_setup:
    /* Keep the odd trailing nibble separately; pairs always begin high. */
    move.w %d2,%d3
    andi.w #1,%d3
    lsr.l #1,%d2
    beq .Ltail
    subq.w #1,%d2
.Lpair:
    moveq #0,%d1
    move.b (%a0)+,%d1
    move.w %d1,%d0
    lsr.w #4,%d0
    lsl.w #2,%d0
    add.w %d5,%d0
    lea (%a2,%d0.w),%a3
    tst.b %d1
    bmi .Lhigh_minus
    add.w (%a3)+,%d4
    bcc .Lhigh_store
    moveq #-16,%d4
    bra .Lhigh_store
.Lhigh_minus:
    sub.w (%a3)+,%d4
    bcc .Lhigh_store
    moveq #0,%d4
.Lhigh_store:
    move.w (%a3),%d5
    move.w %d4,%d0
    eori.w #0x8000,%d0
    move.w %d0,(%a1)+
    move.w %d1,%d0
    andi.w #15,%d0
    lsl.w #2,%d0
    add.w %d5,%d0
    lea (%a2,%d0.w),%a3
    btst #3,%d1
    bne .Llow_minus
    add.w (%a3)+,%d4
    bcc .Llow_store
    moveq #-16,%d4
    bra .Llow_store
.Llow_minus:
    sub.w (%a3)+,%d4
    bcc .Llow_store
    moveq #0,%d4
.Llow_store:
    move.w (%a3),%d5
    move.w %d4,%d0
    eori.w #0x8000,%d0
    move.w %d0,(%a1)+
    dbra %d2,.Lpair
.Ltail:
    tst.w %d3
    beq .Lzeros
    moveq #0,%d1
    move.b (%a0),%d1
    move.w %d1,%d0
    lsr.w #4,%d0
    lsl.w #2,%d0
    add.w %d5,%d0
    lea (%a2,%d0.w),%a3
    tst.b %d1
    bmi .Ltail_minus
    add.w (%a3)+,%d4
    bcc .Ltail_store
    moveq #-16,%d4
    bra .Ltail_store
.Ltail_minus:
    sub.w (%a3)+,%d4
    bcc .Ltail_store
    moveq #0,%d4
.Ltail_store:
    move.w (%a3),%d5
    move.w %d4,%d0
    eori.w #0x8000,%d0
    move.w %d0,(%a1)+
.Lzeros:
    tst.l %d7
    beq .Lcommit
    subq.w #1,%d7
.Lzero_loop:
    clr.w (%a1)+
    dbra %d7,.Lzero_loop
.Lcommit:
    suba.l 44(%sp),%a0
    move.l %a0,(%a4)
    move.l %d6,4(%a4)
    lsr.w #4,%d4
    subi.w #2048,%d4
    move.w %d4,8(%a4)
    lsr.w #6,%d5
    move.w %d5,10(%a4)
    move.b %d3,12(%a4)
.Lreturn:
    movem.l (%sp)+,%d2-%d7/%a2-%a4
    rts
    .section .rodata.wf_sfx_lut,"a"
    .balign 2
wf_sfx_lut:
#include "wf_sfx_lut_q4.inc"
