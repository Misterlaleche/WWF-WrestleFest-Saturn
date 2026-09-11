#include <assert.h>
#include "../../src/runtime/wf_frontend_transition.h"

static void test_fade_table(void)
{
    for(unsigned i=0;i<16;i++) {
        assert(wf_frontend_fade_map_nibble((uint8_t)i,7u)==i);
        assert(wf_frontend_fade_map_nibble((uint8_t)i,0u)==0u);
    }
    assert(wf_frontend_fade_map_nibble(15u,6u)==12u);
    assert(wf_frontend_fade_map_nibble(9u,1u)==2u);
}

static void test_fade_cadence(void)
{
    wf_frontend_transition_t rt;wf_frontend_transition_event_t ev;
    wf_frontend_transition_begin_fade(&rt);
    for(unsigned level=7;;level--){
        for(unsigned i=0;i<3;i++){
            wf_frontend_transition_step_vblank(&rt,&ev);
            assert(ev.valid&&ev.fade_level==level);
            if(level==0u&&i==2u) assert(ev.complete&&!wf_frontend_transition_active(&rt));
            else assert(!ev.complete);
        }
        if(level==0u)break;
    }
}

static void test_stripes(void)
{
    wf_frontend_transition_t rt;wf_frontend_transition_event_t ev;
    wf_frontend_transition_begin_stripe_close(&rt);
    for(unsigned level=0;level<8;level++)for(unsigned i=0;i<3;i++){
        wf_frontend_transition_step_vblank(&rt,&ev);
        assert(ev.stripe_write&&ev.stripe_tile_code==0x04beu+level&&ev.stripe_palette_bank==0x0fu);
    }
    assert(ev.complete&&!ev.stripe_clear_fg0);
    wf_frontend_transition_begin_stripe_open(&rt);
    for(int level=7;level>=0;level--)for(unsigned i=0;i<3;i++){
        wf_frontend_transition_step_vblank(&rt,&ev);
        assert(ev.stripe_write&&ev.stripe_tile_code==(uint16_t)(0x04beu+level));
    }
    assert(ev.complete&&ev.stripe_clear_fg0);
}

static void test_wait_and_slide(void)
{
    wf_frontend_transition_t rt;wf_frontend_transition_event_t ev;
    wf_frontend_transition_begin_wait(&rt,33u);
    for(unsigned i=0;i<32;i++){wf_frontend_transition_step_vblank(&rt,&ev);assert(!ev.complete);}
    wf_frontend_transition_step_vblank(&rt,&ev);assert(ev.complete&&!wf_frontend_transition_active(&rt));
    uint16_t x=0x0140u;
    for(unsigned i=0;i<25;i++) assert(!wf_frontend_select_slide_step(&x,true));
    assert(x==0x0208u);
    assert(wf_frontend_select_slide_step(&x,true));assert(x==0x0210u);
    for(unsigned i=0;i<25;i++) assert(!wf_frontend_select_slide_step(&x,false));
    assert(x==0x0148u);
    assert(wf_frontend_select_slide_step(&x,false));assert(x==0x0140u);
}

int main(void){test_fade_table();test_fade_cadence();test_stripes();test_wait_and_slide();assert(WF_FRONTEND_ATTRACT_POSTDEMO_TOTAL==90u);return 0;}
