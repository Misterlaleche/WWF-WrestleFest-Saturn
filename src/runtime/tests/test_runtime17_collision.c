#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "wf_r17_collision_24062.h"
#include "wf_collision.h"

static void arm(wf_actor_raw_t*a,uint16_t profile,uint16_t x,uint16_t y,uint16_t z,uint8_t facing){
 memset(a,0,sizeof(*a));wf_set_be16(a,WF_A_STATE,0x8000u);wf_set_be16(a,WF_A_PROFILE,profile);wf_set_be16(a,WF_A_X,x);wf_set_be16(a,WF_A_Y,y);wf_set_be16(a,WF_A_Z,z);wf_set_u8(a,WF_A_FACING,facing);
}
int main(void){
 /* Direct ROM rows: profile04 damage byte is 0E, not decimal 14 encoded as 14h. */
 assert(wf_collision_profiles[0x04].pending_damage==0x0eu);
 assert(wf_collision_profiles[0x06].pending_damage==0x0au);
 assert(wf_collision_profiles[0x08].pending_damage==0x10u);
 assert(wf_collision_profiles[0x0f].pending_damage==0x14u);
 wf_runtime_t rt;wf_r17_collision_24062_trace_t tr;memset(&rt,0,sizeof(rt));
 arm(&rt.actors[0],0x0004u,0x0200u,0x0140u,0x0100u,0x00u);
 arm(&rt.actors[1],0x0001u,0x0200u,0x0140u,0x0100u,0x80u);wf_set_u8(&rt.actors[1],WF_A_FLAGS33,0x10u);
 arm(&rt.actors[2],0x0001u,0x0204u,0x0140u,0x0100u,0x80u);wf_set_u8(&rt.actors[2],WF_A_FLAGS33,0x10u);
 assert(wf_r17_collision_24062(&rt,&tr)==WF_OK);
 assert(tr.accepted_pairs==2u); /* retained source A2 row after source+$4C is cleared by first success */
 assert(wf_be16(&rt.actors[0],WF_A_PROFILE)==0u);
 assert((wf_be16(&rt.actors[0],WF_A_COLL_CACHE)&0x6000u)==0x6000u);
 assert(wf_u8(&rt.actors[1],WF_A_PENDING_DAMAGE+1u)==0x0eu);
 assert(wf_u8(&rt.actors[2],WF_A_PENDING_DAMAGE+1u)==0x0eu);
 assert(wf_be32(&rt.actors[0],WF_A_RECIP_LINK)==wf_actor_rom_base(2));
 assert(wf_be32(&rt.actors[1],WF_A_RECIP_LINK)==wf_actor_rom_base(0));
 assert(wf_be32(&rt.actors[2],WF_A_RECIP_LINK)==wf_actor_rom_base(0));
 /* $24DDC ROM branch adds +1 only when +$33.bit4 is SET. */
 memset(&rt,0,sizeof(rt));arm(&rt.actors[0],0x0007u,0x0200u,0x0140u,0x0100u,0x00u);arm(&rt.actors[1],0x0001u,0x0200u,0x0140u,0x0100u,0x00u);wf_set_u8(&rt.actors[1],WF_A_FLAGS33,0x10u);
 assert(wf_r17_collision_24062(&rt,&tr)==WF_OK);assert(wf_be16(&rt.actors[1],WF_A_PENDING_DAMAGE)==2u);
 puts("PASS R17 $24062 exact source/target scan, retained source row, ROM damage bytes and $24DDC polarity");return 0;
}
