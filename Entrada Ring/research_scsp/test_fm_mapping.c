#include "wp01_fm.h"
#include <assert.h>
#include <stdio.h>
static unsigned writes;
static uint16_t regs[512];
static void wr(void *context,uint16_t address,uint16_t value){
    (void)context;assert(address<24*32);assert(!(address&1));
    regs[address/2]=value;++writes;
}
int main(void){
    wp01_fm_init(wr,0);
    /* ALG2: O2 independent, O3 <- O2, O4 <- O1 + O3. */
    wp01_fm_write(0x20,0xc2);
    assert(regs[(1*32+14)/2]==0);
    assert(regs[(2*32+14)/2]==(0xc000U|(31U<<6)|31U));
    assert(regs[(3*32+14)/2]==(0xd000U|(29U<<6)|31U));
    assert(regs[(3*32+22)/2]==0xa000);
    /* YM operator order is [0,16,8,24], including key mask order. */
    wp01_fm_write(0x70,0x12);
    assert(regs[(1*32+12)/2]==0x24);
    wp01_fm_write(0x08,0x10);
    assert((regs[(1*32)/2]&0x800)!=0);
    assert((regs[(2*32)/2]&0x800)==0);
    /* Feedback averages the last two generations, not one sample twice. */
    wp01_fm_write(0x20,0xfa);
    assert(regs[14/2]==(0xb000U|(32U<<6)));
    /* Original channel7 maps to native sixth four-slot group. */
    wp01_fm_write(0x27,0xc4);
    assert(regs[(21*32+22)/2]==0xa000);
    assert(regs[(23*32+22)/2]==0xa000);
    /* The original cue has no active LFO/noise. Flags expose unsupported use. */
    assert(wp01_fm_unsupported()==0);
    wp01_fm_write(0x0f,0x80);
    assert(wp01_fm_unsupported()&WP01_FM_UNSUPPORTED_NOISE);
    printf("PASS: native FM routing, operator order, key mask, feedback, slot isolation (%u writes)\n",writes);
}
