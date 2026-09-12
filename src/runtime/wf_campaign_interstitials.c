#include "wf_campaign_interstitials.h"
#include <string.h>

static const uint8_t k_action_frames[WF_CAMPAIGN_BDA6_ACTION_PHASES]={3u,1u,2u,1u,0u};
static void clear_campaign_event(wf_campaign_event_t *e){if(e)memset(e,0,sizeof(*e));}

bool wf_campaign_b608_should_run(uint8_t m,uint16_t s){return(m&1u)==0u&&s==4u;}

bool wf_campaign_bda6_select(uint8_t m,uint16_t s,uint16_t c,uint8_t*n,uint8_t*b)
{
    uint16_t d;
    if((m&1u)!=0u)return false;
    d=s;
    if((c&0x0080u)!=0u)d=(uint16_t)(d+1u);
    if(d>=5u&&d<=7u)d=(uint16_t)(d-5u);
    else if(d>2u)return false;
    if(n)*n=(uint8_t)d;
    if(b)*b=(uint8_t)(d*7u+4u);
    return true;
}

void wf_campaign_postwin_begin(wf_campaign_postwin_t*r,uint8_t m,uint16_t s,uint8_t p,uint8_t a,uint16_t c)
{
    if(!r)return;
    memset(r,0,sizeof(*r));
    r->mode_0161=m;r->selector_0162=s;r->progression_0163=p;r->auxiliary_0165=a;r->config_0066=c;
    r->stage=WF_CAMPAIGN_STAGE_ENTRY_GATE;
}

void wf_campaign_postwin_step(wf_campaign_postwin_t*r,bool done,wf_campaign_event_t*e)
{
    uint8_t idx,base;
    clear_campaign_event(e);
    if(!r||r->stage==WF_CAMPAIGN_STAGE_DONE)return;
    switch(r->stage){
    case WF_CAMPAIGN_STAGE_ENTRY_GATE:
        if(r->progression_0163==9u){
            r->stage=WF_CAMPAIGN_STAGE_DONE;
            if(e){e->final_completion=true;e->final_completion_rom=WF_CAMPAIGN_FINAL_COMPLETION_ROM;}
            return;
        }
        r->stage=WF_CAMPAIGN_STAGE_B608_GATE;
        return;
    case WF_CAMPAIGN_STAGE_B608_GATE:
        if(wf_campaign_b608_should_run(r->mode_0161,r->selector_0162)){
            r->stage=WF_CAMPAIGN_STAGE_B608_WAIT;
            if(e)e->run_b608=true;
        }else r->stage=WF_CAMPAIGN_STAGE_BDA6_GATE;
        return;
    case WF_CAMPAIGN_STAGE_B608_WAIT:
        if(done)r->stage=WF_CAMPAIGN_STAGE_BDA6_GATE;
        return;
    case WF_CAMPAIGN_STAGE_BDA6_GATE:
        if(wf_campaign_bda6_select(r->mode_0161,r->selector_0162,r->config_0066,&idx,&base)){
            r->stage=WF_CAMPAIGN_STAGE_BDA6_WAIT;
            if(e){e->run_bda6=true;e->bda6_normalized_index=idx;e->bda6_base_frame=base;}
        }else r->stage=WF_CAMPAIGN_STAGE_INCREMENT;
        return;
    case WF_CAMPAIGN_STAGE_BDA6_WAIT:
        if(done)r->stage=WF_CAMPAIGN_STAGE_INCREMENT;
        return;
    case WF_CAMPAIGN_STAGE_INCREMENT:
        r->progression_0163=(uint8_t)(r->progression_0163+1u);
        r->auxiliary_0165=(uint8_t)(r->auxiliary_0165+1u);
        if(e)e->progression_incremented=true;
        if((r->config_0066&0x0080u)!=0u&&(r->progression_0163==3u||r->progression_0163==8u)){
            r->progression_0163=(uint8_t)(r->progression_0163+1u);
            if(e)e->progression_skip_applied=true;
        }
        if(r->progression_0163==5u){
            r->stage=WF_CAMPAIGN_STAGE_PROGRESSION5_PREP_WAIT;
            if(e){
                e->prepare_progression5_table=true;
                e->progression5_table_rom=WF_CAMPAIGN_PROGRESSION5_TABLE_ROM;
                e->progression5_actor_base_rom=WF_CAMPAIGN_PROGRESSION5_ACTOR_BASE_ROM;
                e->progression5_actor_stride=WF_CAMPAIGN_PROGRESSION5_ACTOR_STRIDE;
                e->progression5_actor_field_57=WF_CAMPAIGN_PROGRESSION5_ACTOR_FIELD_57;
                e->progression5_actor_count=WF_CAMPAIGN_PROGRESSION5_ACTOR_COUNT;
            }
        }else r->stage=WF_CAMPAIGN_STAGE_PREMATCH_HANDOFF;
        return;
    case WF_CAMPAIGN_STAGE_PROGRESSION5_PREP_WAIT:
        if(done)r->stage=WF_CAMPAIGN_STAGE_PREMATCH_HANDOFF;
        return;
    case WF_CAMPAIGN_STAGE_PREMATCH_HANDOFF:
        r->stage=WF_CAMPAIGN_STAGE_DONE;
        if(e){e->prematch_handoff=true;e->prematch_rom=WF_CAMPAIGN_NEXT_PREMATCH_ROM;}
        return;
    case WF_CAMPAIGN_STAGE_DONE:
        return;
    }
}

bool wf_bda6_action_expiry(unsigned p,wf_bda6_action_event_t*e){if(e)memset(e,0,sizeof(*e));if(p>=WF_CAMPAIGN_BDA6_ACTION_PHASES)return false;if(e){e->valid=true;e->frame=k_action_frames[p];e->reload_count=WF_CAMPAIGN_BDA6_ACTION_RELOAD;if(p==2u){e->activate_card=true;e->audio_command=true;e->audio_command_word=WF_CAMPAIGN_BDA6_IMPACT_COMMAND;}if(p==4u)e->clear_action_active=true;}return true;}
void wf_bda6_card_begin(wf_bda6_card_state_t*s,uint8_t b){if(!s)return;s->base_frame=b;s->frame=b;s->counter_24=0u;}
bool wf_bda6_card_expiry(wf_bda6_card_state_t*s,wf_bda6_card_event_t*e){uint16_t n;if(e)memset(e,0,sizeof(*e));if(!s||s->counter_24>=WF_CAMPAIGN_BDA6_CARD_TERMINAL)return false;n=(uint16_t)(s->counter_24+1u);s->counter_24=n;if(n<7u){s->frame=(uint8_t)(s->frame+1u);if(e)e->frame_advanced=true;}if(e){e->valid=true;e->reload_count=WF_CAMPAIGN_BDA6_CARD_RELOAD;e->counter_24=s->counter_24;e->frame=s->frame;e->terminal=s->counter_24==WF_CAMPAIGN_BDA6_CARD_TERMINAL;}return true;}
void wf_bda6_sync_begin(wf_bda6_sync_t*s){if(s)memset(s,0,sizeof(*s));}
void wf_bda6_sync_iteration(wf_bda6_sync_t*s){if(!s||s->loop_complete||s->done)return;s->iterations=(uint16_t)(s->iterations+1u);if(s->iterations==WF_CAMPAIGN_BDA6_SYNC_ITERATIONS){s->loop_complete=true;s->fade_remaining=WF_CAMPAIGN_FADE_VBLANKS;}}
void wf_bda6_sync_fade_vblank(wf_bda6_sync_t*s){if(!s||!s->loop_complete||s->done)return;if(s->fade_remaining>0u)s->fade_remaining=(uint8_t)(s->fade_remaining-1u);if(s->fade_remaining==0u)s->done=true;}
