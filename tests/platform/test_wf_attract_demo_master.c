#include <assert.h>
#include <stdint.h>
#include "../../src/runtime/wf_attract_demo_master.h"

static const uint16_t threshold[14]={848,736,1040,928,656,928,1200,944,1616,1152,800,816,832,864};

static void test_table(void)
{
    for(unsigned s=0;s<14;s++){
        const wf_attract_demo_config_t*c=wf_attract_demo_config(s);
        assert(c&&c->state==s&&c->threshold_updates==threshold[s]);
    }
    assert(!wf_attract_demo_config(14));
}

static void test_threshold_and_90_vblank(void)
{
    wf_attract_demo_runtime_t rt;wf_attract_demo_event_t ev;
    assert(wf_attract_demo_begin(&rt,0));
    for(unsigned i=1;i<848;i++){wf_attract_demo_gameplay_update(&rt,&ev);assert(!ev.threshold_reached);}
    wf_attract_demo_gameplay_update(&rt,&ev);
    assert(ev.threshold_reached&&ev.begin_stripe_mask_close);
    assert(ev.stripe_mask_close_rom==0x00008f50u);
    assert(rt.phase==WF_ATTRACT_DEMO_MASK_24);
    for(unsigned i=1;i<=89;i++){wf_attract_demo_transition_vblank(&rt,&ev);assert(!ev.next_attract_state&&!ev.frontend_return);}
    wf_attract_demo_transition_vblank(&rt,&ev);
    assert(ev.next_attract_state&&ev.next_state==1&&rt.phase==WF_ATTRACT_DEMO_HANDOFF);
}

int main(void)
{
    test_table();test_threshold_and_90_vblank();
    assert(WF_ATTRACT_DEMO_TRANSITION_VBLANKS==90u);
    assert(WF_ATTRACT_DEMO_MASK_CLOSE_ROM==0x00008f50u);
    return 0;
}
