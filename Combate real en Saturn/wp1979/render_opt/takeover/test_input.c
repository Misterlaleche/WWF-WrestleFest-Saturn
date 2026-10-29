#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "wf_input.h"

uint8_t normal_input(uint32_t,uint8_t);
uint8_t noinput_input(uint32_t,uint8_t);
extern volatile uint32_t normal_demo_active,noinput_demo_active;

/* Logical fields used by the exact extracted controls() source. This is a
 * host test of the mapper, not a substitute Saturn SMPC layout or driver. */
typedef struct {
 unsigned connected;
 struct {struct {unsigned right,left,up,down,a,b,c,start;} button;} pressed;
} smpc_peripheral_digital_t;
#include "controls.inc"

int main(void){
 assert(normal_demo_active==1u&&noinput_demo_active==0u);
 /* Frozen v4 CSV: southwest until37, south until49, then release. */
 assert(normal_input(0u,0xffu)==0xf9u);
 assert(normal_input(36u,0xffu)==0xf9u);
 assert(normal_input(37u,0xffu)==0xfbu);
 assert(normal_input(49u,0xffu)==0xffu);
 assert(normal_demo_active==1u);
 assert(normal_input(65u,0xfdu)==0xfdu&&normal_demo_active==0u);
 assert(normal_input(66u,0xffu)==0xffu);
 assert(normal_input(72u,0xffu)==0xffu); /* tape would press A here */
 assert(normal_input(80u,0xffu)==0xffu); /* tape would press B here */
 assert(normal_demo_active==0u);
 puts("PASS replay begins with real tape levels; physical LEFT overrides and permanently disables replay; release stays neutral");

 const uint32_t frames[]={0,8,12,65,72,84,540,10000,0xffffffffu};
 for(unsigned i=0;i<sizeof frames/sizeof frames[0];i++)
  assert(noinput_input(frames[i],0xffu)==0xffu&&noinput_demo_active==0u);
 for(unsigned bits=0;bits<256u;bits++){
  assert(noinput_input(10001u+bits,(uint8_t)bits)==bits&&noinput_demo_active==0u);
  assert(normal_input(10001u+bits,(uint8_t)bits)==bits&&normal_demo_active==0u);
 }
 puts("PASS WF_R18_REPLAY_NOINPUT starts neutral at every tape landmark and passes all 256 physical levels unchanged");

 const uint8_t levels[]={0xfe,0xfd,0xf7,0xfb,0xef,0xdf,0xbf,0x7f};
 const uint8_t directions[]={1,2,8,4,0,0,0,0};
 const char *names[]={"RIGHT","LEFT","UP","DOWN","A","B","C","START"};
 for(unsigned i=0;i<8u;i++){
  smpc_peripheral_digital_t pad={0};pad.connected=1u;
  switch(i){
   case 0:pad.pressed.button.right=1;break;case 1:pad.pressed.button.left=1;break;
   case 2:pad.pressed.button.up=1;break;case 3:pad.pressed.button.down=1;break;
   case 4:pad.pressed.button.a=1;break;case 5:pad.pressed.button.b=1;break;
   case 6:pad.pressed.button.c=1;break;case 7:pad.pressed.button.start=1;break;
  }
  uint8_t physical=controls(&pad);assert(physical==levels[i]);
  wf_actor_raw_t actor={{0}};
  wf_input_decode_514e(&actor,noinput_input(i,physical),1u);
  wf_input_decode_514e(&actor,noinput_input(i,physical),0u);
  assert(wf_be16(&actor,WF_A_INPUT_DERIVED)==directions[i]);
  assert(wf_be16(&actor,WF_A_INPUT_HELD)==(i<4u?0u:i==7u?0x100u:1u<<(i-4u)));
  pad.connected=0u;assert(controls(&pad)==0xffu);
  printf("PASS %s physical=0x%02x decoded_direction=0x%x decoded_action=0x%x disconnected=neutral\n",names[i],physical,wf_be16(&actor,WF_A_INPUT_DERIVED),wf_be16(&actor,WF_A_INPUT_HELD));
 }
 return 0;
}
