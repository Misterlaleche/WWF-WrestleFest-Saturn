#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "wf_animation.h"
#include "wf_physics.h"
static void reset(wf_actor_raw_t *a){memset(a,0,sizeof(*a));}
int main(void){
 wf_actor_raw_t a; wf_anim_view_t v; reset(&a);
 assert(wf_anim_desc_count==180u); assert(wf_anim_entry_count==519u);
 /* raw $29 = finite 60 effective ticks: 21+9+15+15 */
 assert(wf_anim_select(&a,WF_ANIM_ACTION,0x29,&v)); unsigned ticks=0; while(!v.terminal && ticks<1000){wf_anim_tick(&a,&v);ticks++;} assert(ticks==60u);
 /* $FF00 is indefinite: raw $23 starts held and neither counter nor index advances. */
 reset(&a); assert(wf_anim_select(&a,WF_ANIM_ACTION,0x23,&v)); assert(wf_be16(&a,WF_A_ANIM_COUNTER)==0xFF00u); for(int i=0;i<1000;i++)wf_anim_tick(&a,&v); assert(wf_be16(&a,WF_A_ANIM_COUNTER)==0xFF00u); assert(wf_be16(&a,WF_A_ANIM_INDEX)==0u);
 /* kind2 main state1 loops; after 44 ticks returns to first entry rather than terminal. */
 reset(&a); assert(wf_anim_select(&a,WF_ANIM_MAIN,1,&v)); for(int i=0;i<44;i++)wf_anim_tick(&a,&v); assert(!v.terminal); assert(wf_be16(&a,WF_A_ANIM_INDEX)==0u);
 /* Physics exact cardinal anchors. */
 reset(&a); wf_set_u8(&a,WF_A_MOTION_MODE,1); wf_set_u8(&a,WF_A_MAGNITUDE+1,1); wf_set_u8(&a,WF_A_ANGLE,0x00); wf_physics_integrate_common(&a); assert(wf_be32(&a,WF_A_X)==0); assert(wf_be32(&a,WF_A_Y)==0x1000u);
 reset(&a); wf_set_u8(&a,WF_A_MOTION_MODE,1); wf_set_u8(&a,WF_A_MAGNITUDE+1,1); wf_set_u8(&a,WF_A_ANGLE,0x40); wf_physics_integrate_common(&a); assert(wf_be32(&a,WF_A_X)==0x1400u); assert(wf_be32(&a,WF_A_Y)==0);
 /* mode2 signed 8.8 -> signed 16.16 delta and WORD decrement. */
 reset(&a); wf_set_u8(&a,WF_A_MOTION_MODE,2); wf_set_be16(&a,WF_A_VX,0x0100); wf_set_be16(&a,WF_A_VY,0xFF00); wf_set_be16(&a,WF_A_VZ,0x0100); wf_set_be16(&a,WF_A_VZ_DEC,0x0038); wf_physics_integrate_common(&a); assert(wf_be32(&a,WF_A_X)==0x00010000u); assert(wf_be32(&a,WF_A_Y)==0xFFFF0000u); assert(wf_be32(&a,WF_A_Z)==0x00010000u); assert(wf_be16(&a,WF_A_VZ)==0x00C8u);
 assert(wf_physics_walk_speed(0,0)==0x1B); assert(wf_physics_walk_speed(0,0x08)==0x0C); assert(wf_physics_run_speed(0)==0x2F);
 puts("PASS test_animation_physics"); return 0;
}
