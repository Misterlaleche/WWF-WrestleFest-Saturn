/* Native integration, not a ROM oracle: the real phase must propagate guards
 * and queue the repaired referee without replacing callbacks or actor data. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "wf_gameplay_bindings.h"
#include "wf_r18_referee.h"
static wf_runtime_t rt;
static wf_gameplay_binding_context_t context;
static void reset(void) {
 memset(&rt,0,sizeof rt); memset(&context,0,sizeof context);
 rt.m68k_regs.valid=1; context.camera_valid=1;
 context.camera.scroll0_x=0x200;context.camera.scroll0_y=0x100;
 context.referee_direction_20c8=wf_r18_referee_direction_20c8;
 wf_set_be16(&rt.referee,WF_A_ACTIVE,0x8000);
 wf_set_be16(&rt.referee,WF_A_CHARACTER,0x000c);
 wf_set_be16(&rt.referee,WF_A_X,0x280);
 wf_set_be16(&rt.referee,WF_A_Y,0x180);
 wf_set_be16(&rt.referee,WF_A_RENDER_LIST,1);
 wf_set_be16(&rt.referee,0x20,9);wf_set_be16(&rt.referee,0x1c,7);
 wf_set_u8(&rt.referee,0x2e,0x7f);
 wf_set_be32(&rt.referee,0x56,wf_actor_rom_base(0));
 rt.g.global_1684=wf_actor_rom_base(0);rt.g.global_1688=wf_actor_rom_base(1);
 for(unsigned i=0;i<WF_ACTOR_SLOTS;i++) {
  wf_set_be16(&rt.actors[i],WF_A_X,(uint16_t)(0x200+32*i));
  wf_set_u8(&rt.actors[i],WF_A_FLAGS32,1);wf_set_u8(&rt.actors[i],WF_A_FLAGS34,0xfd);
 }
}
int main(void) {
 unsigned cases=0;
 const uint32_t pointers[]={0,0x80000000u,0xff000000u,0x001c06bcu};
 for(unsigned i=0;i<4;i++)for(unsigned k=0;k<3;k++) {
  reset();if(k==0)rt.g.global_1688=pointers[i];
  if(k==1)rt.g.global_1684=pointers[i];
  if(k==2)wf_set_be32(&rt.referee,0x56,pointers[i]);
  assert(wf_phase_referee_1f914(&rt,0x1f914,&context)==WF_OK);
  assert(context.last_blocked_phase==0 && context.last_blocked_dependency==0);
  assert(context.render_queue.count[1]==1);
  assert(context.render_queue.list[1][0]==&rt.referee);
  assert((wf_u8(&rt.referee,0x2e)&0x7f)==0);
  assert(wf_u8(&rt.referee,0x23)==9);cases++;
 }
 const uint32_t invalid[]={1,0x001c0500u,0x001c05b2u,0x80000001u};
 const uint32_t dependencies[]={0x1c1688,0x1c1684,0x202d6};
 for(unsigned i=0;i<4;i++)for(unsigned k=0;k<3;k++) {
  reset();if(k==0)rt.g.global_1688=invalid[i];
  if(k==1)rt.g.global_1684=invalid[i];
  if(k==2)wf_set_be32(&rt.referee,0x56,invalid[i]);
  assert(wf_phase_referee_1f914(&rt,0x1f914,&context)==WF_BLOCKED);
  assert(context.last_blocked_phase==0x1f914);
  assert(context.last_blocked_dependency==dependencies[k]);
  assert(context.render_queue.count[1]==0);cases++;
 }
 /* A null target at the primary2 site must reach the existing approach
  * callback, not only pass a direct call to a static leaf. */
 for(unsigned i=0;i<3;i++) {
  reset();wf_set_be16(&rt.referee,0x20,2);wf_set_be16(&rt.referee,0x1c,2);
  rt.g.global_1688=pointers[i];
  assert(wf_phase_referee_1f914(&rt,0x1f914,&context)==WF_OK);
  assert(context.last_blocked_phase==0);
  assert(wf_be32(&rt.referee,0x56)==pointers[i]);cases++;
 }
 /* Referee timer keeps running for 120 consecutive phase calls after
  * accepting an empty target; native integration, not cycle certification. */
 reset();rt.g.global_1688=0;unsigned switches=0;uint8_t last=0;
 for(unsigned tick=0;tick<120;tick++) {
  wf_r17_render_queue_clear(&context.render_queue);
  assert(wf_phase_referee_1f914(&rt,0x1f914,&context)==WF_OK);
  uint8_t pose=wf_u8(&rt.referee,5);assert(pose==7||pose==8);
  if(tick&&pose!=last)switches++;
  last=pose;
 }
 assert(switches>=8);cases++;
 printf("{\"status\":\"PASS_NATIVE_PHASE\",\"cases\":%u,\"continuous_ticks\":120}\n",cases);
 return 0;
}
