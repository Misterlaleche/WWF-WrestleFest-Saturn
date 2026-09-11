#include <assert.h>
#include "../../src/runtime/wf_campaign_interstitials.h"

static void gates(void)
{
    assert(wf_campaign_b608_should_run(0u,4u));
    assert(!wf_campaign_b608_should_run(1u,4u));
    assert(!wf_campaign_b608_should_run(0u,3u));
    for(unsigned d=0;d<10;d++){
        uint8_t i=0xff,b=0xff;
        bool ok=wf_campaign_bda6_select(0u,(uint16_t)d,0u,&i,&b);
        bool want=(d<=2u)||(d>=5u&&d<=7u);
        assert(ok==want);
        if(ok){unsigned n=d>=5u?d-5u:d;assert(i==n&&b==n*7u+4u);}
    }
    assert(!wf_campaign_bda6_select(1u,0u,0u,0,0));
    {uint8_t i,b;assert(wf_campaign_bda6_select(0u,4u,0x0080u,&i,&b));assert(i==0u&&b==4u);}
    {uint8_t i,b;assert(!wf_campaign_bda6_select(0u,2u,0x0080u,&i,&b));}
}

static void action_card(void)
{
    static const uint8_t frames[5]={3,1,2,1,0};
    wf_bda6_action_event_t a;
    for(unsigned p=0;p<5;p++){
        assert(wf_bda6_action_expiry(p,&a));assert(a.valid&&a.frame==frames[p]&&a.reload_count==10u);
        assert(a.audio_command==(p==2u));assert(a.activate_card==(p==2u));assert(a.clear_action_active==(p==4u));
        if(p==2u)assert(a.audio_command_word==0x312au);
    }
    assert(!wf_bda6_action_expiry(5,&a));
    wf_bda6_card_state_t s;wf_bda6_card_event_t e;wf_bda6_card_begin(&s,11u);
    for(unsigned n=1;n<=16;n++){
        assert(wf_bda6_card_expiry(&s,&e));assert(e.counter_24==n&&e.reload_count==8u);
        if(n<7u)assert(e.frame_advanced&&e.frame==11u+n);else assert(!e.frame_advanced&&e.frame==17u);
        assert(e.terminal==(n==16u));
    }
    assert(!wf_bda6_card_expiry(&s,&e));
}

static void timing(void)
{
    wf_bda6_sync_t s;wf_bda6_sync_begin(&s);
    for(unsigned i=1;i<142;i++){wf_bda6_sync_iteration(&s);assert(!s.loop_complete&&s.iterations==i);}
    wf_bda6_sync_iteration(&s);assert(s.loop_complete&&s.iterations==142u&&s.fade_remaining==24u);
    for(unsigned i=0;i<23;i++){wf_bda6_sync_fade_vblank(&s);assert(!s.done);}
    wf_bda6_sync_fade_vblank(&s);assert(s.done&&s.fade_remaining==0u);
}

static void ordering(void)
{
    wf_campaign_postwin_t r;wf_campaign_event_t e;
    wf_campaign_postwin_begin(&r,0u,4u,2u,7u,0u);
    wf_campaign_postwin_step(&r,false,&e);assert(e.run_b608&&!e.run_bda6&&r.progression_0163==2u);
    wf_campaign_postwin_step(&r,true,&e);assert(r.stage==WF_CAMPAIGN_STAGE_BDA6_GATE&&r.progression_0163==2u);
    wf_campaign_postwin_step(&r,false,&e);assert(!e.run_b608&&!e.run_bda6&&r.stage==WF_CAMPAIGN_STAGE_INCREMENT);
    wf_campaign_postwin_step(&r,false,&e);assert(e.progression_incremented&&r.progression_0163==3u&&r.auxiliary_0165==8u);
    wf_campaign_postwin_step(&r,false,&e);assert(e.prematch_handoff&&e.prematch_rom==0x00000ac0u);

    wf_campaign_postwin_begin(&r,1u,0u,8u,3u,0u);
    wf_campaign_postwin_step(&r,false,&e);assert(!e.run_b608&&r.stage==WF_CAMPAIGN_STAGE_BDA6_GATE);
    wf_campaign_postwin_step(&r,false,&e);assert(!e.run_bda6&&r.stage==WF_CAMPAIGN_STAGE_INCREMENT);
}

int main(void){gates();action_card();timing();ordering();return 0;}
