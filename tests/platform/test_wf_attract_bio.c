#include <assert.h>
#include "../../src/runtime/wf_attract_bio.h"

static void mapping(void)
{
    const int want[14]={-1,-1,0,1,2,3,6,7,8,9,10,11,4,5};
    for(unsigned s=0;s<14;s++){
        unsigned f=99;
        bool ok=wf_attract_bio_fighter_for_state(s,&f);
        assert(ok==(want[s]>=0));
        if(ok)assert(f==(unsigned)want[s]);
    }
}

static void lifecycle(void)
{
    wf_attract_bio_t b;
    wf_attract_bio_begin(&b,2);
    assert(!b.cleaned && b.fighter==WF_BIO_HOGAN);
    wf_attract_bio_step(&b,0,0);
    assert(wf_attract_bio_frame(&b)==0 && b.dwell==7 && wf_attract_bio_portrait_visible(&b) && wf_attract_bio_text_visible(&b));
    for(unsigned t=1;t<=7;t++) wf_attract_bio_step(&b,(uint16_t)t,0);
    assert(wf_attract_bio_frame(&b)==0 && b.toggle==1 && b.dwell==8);
    wf_attract_bio_step(&b,8,15);
    assert(wf_attract_bio_frame(&b)==1 && b.dwell==7);
    for(unsigned t=9;t<256;t++){
        uint8_t r=(b.dwell==1)?0:9;
        wf_attract_bio_step(&b,(uint16_t)t,r);
    }
    wf_attract_bio_step(&b,256,0);
    assert(wf_attract_bio_frame(&b)==2);
    for(unsigned t=257;t<=511;t++) wf_attract_bio_step(&b,(uint16_t)t,0);
    assert(wf_attract_bio_portrait_visible(&b) && wf_attract_bio_text_visible(&b) && !b.cleaned);
    wf_attract_bio_step(&b,512,0);
    assert(wf_attract_bio_portrait_visible(&b) && !wf_attract_bio_text_visible(&b) && b.cleaned);
    wf_attract_bio_step(&b,513,0);
    assert(!wf_attract_bio_portrait_visible(&b) && !wf_attract_bio_text_visible(&b));
}

int main(void)
{
    mapping();
    lifecycle();
    assert(wf_attract_bio_dwell_from_nibble(0)==8);
    assert(wf_attract_bio_dwell_from_nibble(15)==23);
    assert(WF_ATTRACT_BIO_PACK_BYTES==37696u);
    return 0;
}
