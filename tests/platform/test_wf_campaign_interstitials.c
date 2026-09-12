#include <assert.h>
#include "../../src/runtime/wf_campaign_interstitials.h"

static void advance_no_scenes(wf_campaign_postwin_t*r,wf_campaign_event_t*e)
{
    wf_campaign_postwin_step(r,false,e);assert(r->stage==WF_CAMPAIGN_STAGE_B608_GATE);
    wf_campaign_postwin_step(r,false,e);assert(r->stage==WF_CAMPAIGN_STAGE_BDA6_GATE);
    wf_campaign_postwin_step(r,false,e);assert(r->stage==WF_CAMPAIGN_STAGE_INCREMENT);
}

static void final_gate(void)
{
    wf_campaign_postwin_t r;wf_campaign_event_t e;
    wf_campaign_postwin_begin(&r,0,0,9,2,0);
    wf_campaign_postwin_step(&r,false,&e);
    assert(e.final_completion&&e.final_completion_rom==0x1b4eu&&r.stage==WF_CAMPAIGN_STAGE_DONE);
    assert(!e.run_b608&&!e.run_bda6&&!e.progression_incremented);
}

static void skip_adjust(void)
{
    wf_campaign_postwin_t r;wf_campaign_event_t e;
    wf_campaign_postwin_begin(&r,1,0,2,7,0x80);
    advance_no_scenes(&r,&e);
    wf_campaign_postwin_step(&r,false,&e);
    assert(e.progression_incremented&&e.progression_skip_applied);
    assert(r.progression_0163==4&&r.auxiliary_0165==8&&r.stage==WF_CAMPAIGN_STAGE_PREMATCH_HANDOFF);
}

static void progression5_prep(void)
{
    wf_campaign_postwin_t r;wf_campaign_event_t e;
    wf_campaign_postwin_begin(&r,1,0,4,0,0);
    advance_no_scenes(&r,&e);
    wf_campaign_postwin_step(&r,false,&e);
    assert(r.progression_0163==5&&r.stage==WF_CAMPAIGN_STAGE_PROGRESSION5_PREP_WAIT);
    assert(e.prepare_progression5_table);
    assert(e.progression5_table_rom==0x001c0598u&&e.progression5_actor_base_rom==0x001c05b0u);
    assert(e.progression5_actor_stride==0x010cu&&e.progression5_actor_field_57==0x57u&&e.progression5_actor_count==4u);
    wf_campaign_postwin_step(&r,false,&e);
    assert(r.stage==WF_CAMPAIGN_STAGE_PROGRESSION5_PREP_WAIT&&!e.prematch_handoff);
    wf_campaign_postwin_step(&r,true,&e);
    assert(r.stage==WF_CAMPAIGN_STAGE_PREMATCH_HANDOFF);
    wf_campaign_postwin_step(&r,false,&e);
    assert(e.prematch_handoff&&e.prematch_rom==0x0ac0u);
}

static void bda6(void)
{
    wf_bda6_action_event_t a;static const uint8_t f[5]={3,1,2,1,0};
    for(unsigned p=0;p<5;p++){assert(wf_bda6_action_expiry(p,&a));assert(a.frame==f[p]);}
    wf_bda6_sync_t s;wf_bda6_sync_begin(&s);
    for(unsigned i=0;i<142;i++)wf_bda6_sync_iteration(&s);
    assert(s.loop_complete&&s.fade_remaining==24);
    for(unsigned i=0;i<24;i++)wf_bda6_sync_fade_vblank(&s);
    assert(s.done);
}

int main(void){final_gate();skip_adjust();progression5_prep();bda6();return 0;}
