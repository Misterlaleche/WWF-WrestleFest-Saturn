#include <string.h>
#include "wf_runtime.h"
#include "wf_collision_dispatch.h"
#include "wf_collision.h"
static uint16_t r16(const uint8_t*p){return (uint16_t)((p[0]<<8)|p[1]);}
static void w(uint8_t*p,uint32_t value,unsigned n){for(unsigned i=0;i<n;i++)p[i]=(uint8_t)(value>>(8u*(n-1u-i)));}
unsigned collision_24d5e_globals_size(void){return sizeof(wf_globals_raw_t);}
void collision_24d5e_project(uint8_t*ram,const uint8_t*globals,const uint8_t*actors){
 wf_globals_raw_t g;memcpy(&g,globals,sizeof g);memset(ram,0,0x2000);memcpy(ram+0x5b0,actors,9*WF_ACTOR_BYTES);
 w(ram+0x5c,g.rng_state,4);w(ram+0x80,g.frame_counter,4);w(ram+0x7c,g.credited_route,2);w(ram+0x161,g.mode_0161,1);
}
int collision_24d5e_oracle(uint8_t*mem,uint32_t*d,unsigned ss,unsigned ts,unsigned ps,unsigned pt){
 (void)d;wf_runtime_t r={0};uint8_t*g=mem+0x1c0000;memcpy(r.actors,g+0x5b0,sizeof r.actors);
 wf_actor_raw_t*s=&r.actors[ss],*t=&r.actors[ts];uint32_t link=((uint32_t)r16(t->b+0x26)<<16)|r16(t->b+0x28);
 wf_collision_pair_ctx_t c={s,t,wf_actor_from_rom_ptr(&r,link,NULL),(uint16_t)(ts+1u),(uint8_t)ps,(uint8_t)pt,NULL};
 wf_coll_result_t result=wf_collision_profile_pair_apply_rows((uint8_t)ps,(uint8_t)pt,&c);memcpy(g+0x5b0,r.actors,sizeof r.actors);return result;
}

void collision_24d5e_norm(uint8_t*mem,uint32_t*d,unsigned ss,unsigned ts){
 (void)d;wf_runtime_t r={0};uint8_t*g=mem+0x1c0000;memcpy(r.actors,g+0x5b0,sizeof r.actors);
 wf_collision_normalize_receiver(&r.actors[ss],&r.actors[ts]);memcpy(g+0x5b0,r.actors,sizeof r.actors);
}
