#include <stddef.h>
#include "wf_runtime.h"
void wf_action41_layout(uint32_t out[11]){
 out[0]=(uint32_t)offsetof(wf_runtime_t,actors);out[1]=WF_ACTOR_BYTES;
 out[2]=(uint32_t)offsetof(wf_runtime_t,g.rng_state);out[3]=(uint32_t)offsetof(wf_runtime_t,g.frame_counter);
 out[4]=(uint32_t)offsetof(wf_runtime_t,g.mode_0161);out[5]=(uint32_t)offsetof(wf_runtime_t,g.config_0066);
 out[6]=(uint32_t)offsetof(wf_runtime_t,g.credited_route);out[7]=(uint32_t)offsetof(wf_runtime_t,g.global_1150);
 out[8]=(uint32_t)offsetof(wf_runtime_t,g.global_1180);out[9]=(uint32_t)offsetof(wf_runtime_t,g.global_11b0);out[10]=(uint32_t)offsetof(wf_runtime_t,g.global_11e0);
}
