#include "wf_attract_demo_master.h"
#include <string.h>

static const wf_attract_demo_config_t k_configs[WF_ATTRACT_DEMO_STATE_COUNT] = {
    {0x00u,848u,WF_ATTRACT_DEMO_TAG,4u,{0x00u,0x01u,0x04u,0x05u,0xFFu,0xFFu},WF_ATTRACT_DEMO_NO_BIO,WF_ATTRACT_OVERRIDE_STATE0_SPECIAL,0u},
    {0x01u,736u,WF_ATTRACT_DEMO_RUMBLE,6u,{0x03u,0x06u,0x07u,0x08u,0x09u,0x02u},WF_ATTRACT_DEMO_NO_BIO,WF_ATTRACT_OVERRIDE_DISABLED_RUMBLE,WF_ATTRACT_SETUP_SPECIAL_01_02},
    {0x02u,1040u,WF_ATTRACT_DEMO_TAG,4u,{0x00u,0x01u,0x06u,0x07u,0xFFu,0xFFu},0x00u,WF_ATTRACT_OVERRIDE_CHARACTER_12,WF_ATTRACT_SETUP_SPECIAL_01_02},
    {0x03u,928u,WF_ATTRACT_DEMO_TAG,4u,{0x01u,0x00u,0x09u,0x08u,0xFFu,0xFFu},0x01u,WF_ATTRACT_OVERRIDE_CHARACTER_12,0u},
    {0x04u,656u,WF_ATTRACT_DEMO_TAG,4u,{0x02u,0x03u,0x0Au,0x0Bu,0xFFu,0xFFu},0x02u,WF_ATTRACT_OVERRIDE_CHARACTER_12,0u},
    {0x05u,928u,WF_ATTRACT_DEMO_TAG,4u,{0x03u,0x02u,0x06u,0x09u,0xFFu,0xFFu},0x03u,WF_ATTRACT_OVERRIDE_CHARACTER_12,0u},
    {0x06u,1200u,WF_ATTRACT_DEMO_TAG,4u,{0x06u,0x07u,0x00u,0x01u,0xFFu,0xFFu},0x06u,WF_ATTRACT_OVERRIDE_CHARACTER_12,WF_ATTRACT_SETUP_PREHOOK_8490},
    {0x07u,944u,WF_ATTRACT_DEMO_TAG,4u,{0x07u,0x09u,0x02u,0x00u,0xFFu,0xFFu},0x07u,WF_ATTRACT_OVERRIDE_CHARACTER_12,0u},
    {0x08u,1616u,WF_ATTRACT_DEMO_TAG,4u,{0x08u,0x06u,0x03u,0x02u,0xFFu,0xFFu},0x08u,WF_ATTRACT_OVERRIDE_CHARACTER_12,0u},
    {0x09u,1152u,WF_ATTRACT_DEMO_TAG,4u,{0x09u,0x08u,0x01u,0x03u,0xFFu,0xFFu},0x09u,WF_ATTRACT_OVERRIDE_CHARACTER_12,0u},
    {0x0Au,800u,WF_ATTRACT_DEMO_TAG,4u,{0x0Au,0x0Bu,0x02u,0x01u,0xFFu,0xFFu},0x0Au,WF_ATTRACT_OVERRIDE_CHARACTER_12,WF_ATTRACT_SETUP_PREHOOK_8490},
    {0x0Bu,816u,WF_ATTRACT_DEMO_TAG,4u,{0x0Bu,0x0Au,0x02u,0x01u,0xFFu,0xFFu},0x0Bu,WF_ATTRACT_OVERRIDE_CHARACTER_12,0u},
    {0x0Cu,832u,WF_ATTRACT_DEMO_TAG,4u,{0x04u,0x05u,0x00u,0x01u,0xFFu,0xFFu},0x04u,WF_ATTRACT_OVERRIDE_CHARACTER_12,0u},
    {0x0Du,864u,WF_ATTRACT_DEMO_TAG,4u,{0x05u,0x04u,0x00u,0x01u,0xFFu,0xFFu},0x05u,WF_ATTRACT_OVERRIDE_CHARACTER_12,0u}
};

static void clear_event(wf_attract_demo_event_t *event){if(event)memset(event,0,sizeof(*event));}
const wf_attract_demo_config_t *wf_attract_demo_config(unsigned state){if(state>=WF_ATTRACT_DEMO_STATE_COUNT)return 0;return &k_configs[state];}
bool wf_attract_demo_begin(wf_attract_demo_runtime_t *rt,unsigned state){const wf_attract_demo_config_t *cfg;if(!rt)return false;cfg=wf_attract_demo_config(state);if(!cfg)return false;memset(rt,0,sizeof(*rt));rt->config=cfg;rt->phase=WF_ATTRACT_DEMO_GAMEPLAY;return true;}
void wf_attract_demo_gameplay_update(wf_attract_demo_runtime_t *rt,wf_attract_demo_event_t *event){clear_event(event);if(!rt||!rt->config||rt->phase!=WF_ATTRACT_DEMO_GAMEPLAY)return;rt->gameplay_updates++;if(rt->gameplay_updates==(uint32_t)rt->config->threshold_updates){rt->phase=WF_ATTRACT_DEMO_MASK_24;rt->phase_vblanks_remaining=WF_ATTRACT_DEMO_MASK_VBLANKS;if(event){event->threshold_reached=true;event->begin_stripe_mask_close=true;event->stripe_mask_close_rom=WF_ATTRACT_DEMO_MASK_CLOSE_ROM;}}}
void wf_attract_demo_transition_vblank(wf_attract_demo_runtime_t *rt,wf_attract_demo_event_t *event){clear_event(event);if(!rt||!rt->config)return;if(rt->phase==WF_ATTRACT_DEMO_GAMEPLAY||rt->phase==WF_ATTRACT_DEMO_HANDOFF)return;if(rt->phase_vblanks_remaining>0u)--rt->phase_vblanks_remaining;if(rt->phase_vblanks_remaining!=0u)return;if(rt->phase==WF_ATTRACT_DEMO_MASK_24){rt->phase=WF_ATTRACT_DEMO_WAIT_33_A;rt->phase_vblanks_remaining=WF_ATTRACT_DEMO_WAIT_A_VBLANKS;return;}if(rt->phase==WF_ATTRACT_DEMO_WAIT_33_A){rt->phase=WF_ATTRACT_DEMO_WAIT_33_B;rt->phase_vblanks_remaining=WF_ATTRACT_DEMO_WAIT_B_VBLANKS;return;}if(rt->phase==WF_ATTRACT_DEMO_WAIT_33_B){rt->phase=WF_ATTRACT_DEMO_HANDOFF;if(rt->config->state<WF_ATTRACT_DEMO_STATE_LAST){if(event){event->next_attract_state=true;event->next_state=(uint8_t)(rt->config->state+1u);}}else if(event){event->frontend_return=true;event->frontend_return_rom=WF_ATTRACT_DEMO_FRONTEND_RETURN_ROM;}}}
bool wf_attract_demo_is_live_gameplay(const wf_attract_demo_runtime_t *rt){return rt&&rt->config&&rt->phase==WF_ATTRACT_DEMO_GAMEPLAY;}
bool wf_attract_demo_credited_start_is_external(void){return true;}
