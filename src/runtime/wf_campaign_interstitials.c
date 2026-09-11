#include "wf_campaign_interstitials.h"

#include <string.h>

static const uint8_t k_action_frames[WF_CAMPAIGN_BDA6_ACTION_PHASES] = {3u,1u,2u,1u,0u};

static void clear_campaign_event(wf_campaign_event_t *event)
{
    if (event) memset(event,0,sizeof(*event));
}

bool wf_campaign_b608_should_run(uint8_t mode_0161, uint16_t selector_0162)
{
    return (mode_0161 & 0x01u) == 0u && selector_0162 == 4u;
}

bool wf_campaign_bda6_select(uint8_t mode_0161,
                             uint16_t selector_0162,
                             uint16_t config_0066,
                             uint8_t *normalized_index,
                             uint8_t *base_frame)
{
    uint16_t d;
    if ((mode_0161 & 0x01u) != 0u) return false;
    d=selector_0162;
    if ((config_0066 & 0x0080u) != 0u) d=(uint16_t)(d+1u);
    if (d>=5u && d<=7u) d=(uint16_t)(d-5u);
    else if (d>2u) return false;
    if (normalized_index) *normalized_index=(uint8_t)d;
    if (base_frame) *base_frame=(uint8_t)(d*7u+4u);
    return true;
}

void wf_campaign_postwin_begin(wf_campaign_postwin_t *rt,
                               uint8_t mode_0161,
                               uint16_t selector_0162,
                               uint8_t progression_0163,
                               uint8_t auxiliary_0165,
                               uint16_t config_0066)
{
    if (!rt) return;
    memset(rt,0,sizeof(*rt));
    rt->mode_0161=mode_0161;
    rt->selector_0162=selector_0162;
    rt->progression_0163=progression_0163;
    rt->auxiliary_0165=auxiliary_0165;
    rt->config_0066=config_0066;
    rt->stage=WF_CAMPAIGN_STAGE_B608_GATE;
}

void wf_campaign_postwin_step(wf_campaign_postwin_t *rt,
                              bool current_scene_complete,
                              wf_campaign_event_t *event)
{
    uint8_t idx,base;
    clear_campaign_event(event);
    if (!rt || rt->stage==WF_CAMPAIGN_STAGE_DONE) return;

    switch (rt->stage) {
    case WF_CAMPAIGN_STAGE_B608_GATE:
        if (wf_campaign_b608_should_run(rt->mode_0161,rt->selector_0162)) {
            rt->stage=WF_CAMPAIGN_STAGE_B608_WAIT;
            if (event) event->run_b608=true;
        } else {
            rt->stage=WF_CAMPAIGN_STAGE_BDA6_GATE;
        }
        return;

    case WF_CAMPAIGN_STAGE_B608_WAIT:
        if (current_scene_complete) rt->stage=WF_CAMPAIGN_STAGE_BDA6_GATE;
        return;

    case WF_CAMPAIGN_STAGE_BDA6_GATE:
        if (wf_campaign_bda6_select(rt->mode_0161,rt->selector_0162,rt->config_0066,&idx,&base)) {
            rt->stage=WF_CAMPAIGN_STAGE_BDA6_WAIT;
            if (event) {
                event->run_bda6=true;
                event->bda6_normalized_index=idx;
                event->bda6_base_frame=base;
            }
        } else {
            rt->stage=WF_CAMPAIGN_STAGE_INCREMENT;
        }
        return;

    case WF_CAMPAIGN_STAGE_BDA6_WAIT:
        if (current_scene_complete) rt->stage=WF_CAMPAIGN_STAGE_INCREMENT;
        return;

    case WF_CAMPAIGN_STAGE_INCREMENT:
        rt->progression_0163=(uint8_t)(rt->progression_0163+1u);
        rt->auxiliary_0165=(uint8_t)(rt->auxiliary_0165+1u);
        rt->stage=WF_CAMPAIGN_STAGE_PREMATCH_HANDOFF;
        if (event) event->progression_incremented=true;
        return;

    case WF_CAMPAIGN_STAGE_PREMATCH_HANDOFF:
        rt->stage=WF_CAMPAIGN_STAGE_DONE;
        if (event) {
            event->prematch_handoff=true;
            event->prematch_rom=WF_CAMPAIGN_NEXT_PREMATCH_ROM;
        }
        return;

    case WF_CAMPAIGN_STAGE_DONE:
        return;
    }
}

bool wf_bda6_action_expiry(unsigned phase, wf_bda6_action_event_t *event)
{
    if (event) memset(event,0,sizeof(*event));
    if (phase>=WF_CAMPAIGN_BDA6_ACTION_PHASES) return false;
    if (event) {
        event->valid=true;
        event->frame=k_action_frames[phase];
        event->reload_count=WF_CAMPAIGN_BDA6_ACTION_RELOAD;
        if (phase==2u) {
            event->activate_card=true;
            event->audio_command=true;
            event->audio_command_word=WF_CAMPAIGN_BDA6_IMPACT_COMMAND;
        }
        if (phase==4u) event->clear_action_active=true;
    }
    return true;
}

void wf_bda6_card_begin(wf_bda6_card_state_t *state,uint8_t base_frame)
{
    if (!state) return;
    state->base_frame=base_frame;
    state->frame=base_frame;
    state->counter_24=0u;
}

bool wf_bda6_card_expiry(wf_bda6_card_state_t *state,wf_bda6_card_event_t *event)
{
    uint16_t next;
    if (event) memset(event,0,sizeof(*event));
    if (!state || state->counter_24>=WF_CAMPAIGN_BDA6_CARD_TERMINAL) return false;
    next=(uint16_t)(state->counter_24+1u);
    state->counter_24=next;
    if (next<7u) {
        state->frame=(uint8_t)(state->frame+1u);
        if (event) event->frame_advanced=true;
    }
    if (event) {
        event->valid=true;
        event->reload_count=WF_CAMPAIGN_BDA6_CARD_RELOAD;
        event->counter_24=state->counter_24;
        event->frame=state->frame;
        event->terminal=state->counter_24==WF_CAMPAIGN_BDA6_CARD_TERMINAL;
    }
    return true;
}

void wf_bda6_sync_begin(wf_bda6_sync_t *sync)
{
    if (!sync) return;
    memset(sync,0,sizeof(*sync));
}

void wf_bda6_sync_iteration(wf_bda6_sync_t *sync)
{
    if (!sync || sync->loop_complete || sync->done) return;
    sync->iterations=(uint16_t)(sync->iterations+1u);
    if (sync->iterations==WF_CAMPAIGN_BDA6_SYNC_ITERATIONS) {
        sync->loop_complete=true;
        sync->fade_remaining=WF_CAMPAIGN_FADE_VBLANKS;
    }
}

void wf_bda6_sync_fade_vblank(wf_bda6_sync_t *sync)
{
    if (!sync || !sync->loop_complete || sync->done) return;
    if (sync->fade_remaining>0u) sync->fade_remaining=(uint8_t)(sync->fade_remaining-1u);
    if (sync->fade_remaining==0u) sync->done=true;
}
