#include "wf_combat.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static wf_session_t s;static wf_runtime_t before;static wf_combat_t c;
static wf_status_t rom(void*u,uint32_t a,void*p,size_t n){(void)u;(void)a;(void)p;(void)n;return WF_BLOCKED;}
static wf_status_t input(wf_runtime_t*r,uint32_t a,uint16_t*v,void*u){(void)r;(void)a;(void)v;(void)u;return WF_BLOCKED;}
static wf_status_t word(wf_runtime_t*r,uint16_t a,void*u){(void)r;(void)a;(void)u;return WF_OK;}
static wf_status_t transition(wf_runtime_t*r,uint8_t a,void*u){return word(r,a,u);}
static wf_status_t render(wf_runtime_t*r,wf_actor_raw_t*const*p,size_t n,void*u){(void)r;(void)p;(void)n;(void)u;return WF_OK;}
int main(void){
 wf_combat_io_t io={rom,input,word,word,render,transition,transition};
 assert(wf_combat_join(&c,&s,&io)==WF_BAD_STATE);
 s.initialized=1;s.phase=WF_SESSION_GAMEPLAY;s.runtime.frame_stage=3;s.runtime.g.rng_state=0x12345678;
 s.runtime.g.global_007e=2;s.runtime.g.global_17e6=0x345;s.runtime.g.global_17ee=0x456;
 for(unsigned i=0;i<256;i++)s.ram[0x16d6+i]=(uint8_t)i;
 before=s.runtime;assert(wf_combat_join(&c,&s,&io)==WF_OK);
 before.bindings=c.bindings;before.binding_count=36;
 assert(!memcmp(&before,&s.runtime,sizeof before));
 assert(c.world.cache[0][0].source==0x00010203&&c.world.cache[1][15].destination==0xfcfdfeff);
 assert(!c.world.animation_count[0]&&!c.world.animation_count[1]);
 assert(c.gameplay.camera.scene_index==2&&c.gameplay.camera.scroll0_x==0x345&&c.gameplay.camera.scroll0_y==0x456);
 assert(c.gameplay.r17_owner_user==&s.video&&s.video.read_rom==rom);
 uint16_t v;s.runtime.g.credit_remainder_004e=3;s.runtime.g.credit_count_004f=5;
 assert(wf_combat_read_ram(&s.runtime,0x1c004e,&v,&s.video)==WF_OK&&v==0x305);
 assert(wf_combat_read_ram(&s.runtime,0x1c1234,&v,&s.video)==WF_BLOCKED&&s.video.failed_address==0x1c1234);
 puts("combat join: exact F8C state retained, 36 bindings, world transports and live RAM PASS");return 0;
}
