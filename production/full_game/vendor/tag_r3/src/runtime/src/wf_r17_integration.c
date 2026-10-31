#include "wf_r17_integration.h"
wf_status_t wf_r17_runtime_enter_f8c(wf_runtime_t *rt,wf_r17_f8c_trace_t *tr){
 wf_r17_f8c_state_t s;wf_status_t st;
 if(!rt||!rt->g.rng_state_valid||!rt->m68k_regs.valid)return WF_BLOCKED;
 s.control_1c0076=(uint16_t)((uint16_t)rt->g.gameplay_vblank_enable<<8);
 s.sync_1c006e=(uint16_t)(((uint16_t)rt->g.frame_handshake[0]<<8)|rt->g.frame_handshake[1]);
 s.counter_1c0080=rt->g.frame_counter;s.branch_1c0083=rt->g.pipeline_mode;
 st=wf_r17_f8c_enter(&s,&rt->g.rng_state,&rt->m68k_regs,tr);if(st!=WF_OK)return st;
 rt->g.gameplay_vblank_enable=(uint8_t)(s.control_1c0076>>8);
 rt->g.frame_handshake[0]=(uint8_t)(s.sync_1c006e>>8);rt->g.frame_handshake[1]=(uint8_t)s.sync_1c006e;
 rt->g.frame_counter=s.counter_1c0080;rt->g.rng_register_context_valid=rt->m68k_regs.valid;
 return WF_OK;
}
