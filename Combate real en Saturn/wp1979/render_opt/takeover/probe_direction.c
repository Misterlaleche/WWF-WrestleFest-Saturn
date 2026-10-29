#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "wf_input.h"
#include "wf_physics.h"
typedef struct {
 unsigned connected;
 struct {struct {unsigned right,left,up,down,a,b,c,start;} button;} pressed;
} smpc_peripheral_digital_t;
#include "controls-baseline.inc"
#include "controls-proposed.inc"
static uint8_t rom[524288];
int main(int argc,char **argv){
 assert(argc==3);FILE *f=fopen(argv[1],"rb");assert(f);
 assert(fread(rom,1,sizeof rom,f)==sizeof rom);fclose(f);
 unsigned proposed=!strcmp(argv[2],"proposed"),errors=0;
 const char *names[]={"RIGHT","LEFT","UP","DOWN"};
 const int32_t desired[4][2]={{138240,0},{-138240,0},{0,-110592},{0,110592}};
 for(unsigned i=0;i<4u;i++){
  smpc_peripheral_digital_t pad={0};pad.connected=1u;
  if(i==0)pad.pressed.button.right=1;if(i==1)pad.pressed.button.left=1;
  if(i==2)pad.pressed.button.up=1;if(i==3)pad.pressed.button.down=1;
  uint8_t low=proposed?proposed_controls(&pad):baseline_controls(&pad);
  wf_actor_raw_t actor={{0}};wf_input_decode_514e(&actor,low,1u);
  unsigned index=wf_be16(&actor,WF_A_INPUT_DERIVED);
  uint16_t angle=wf_input_direction_state1((uint8_t)index);
  uint16_t rom_angle=(uint16_t)((rom[0xf2d6u+index*2u]<<8)|rom[0xf2d7u+index*2u]);
  assert(angle==rom_angle);
  wf_set_be16(&actor,WF_A_ANGLE_WORD,angle);
  wf_set_u8(&actor,WF_A_MOTION_MODE,1u);
  wf_set_u8(&actor,WF_A_MAGNITUDE+1u,wf_physics_walk_speed(0,0));
  assert(wf_physics_walk_speed(0,0)==rom[0x116aeu]);
  wf_set_be32(&actor,WF_A_X,0x01000000u);wf_set_be32(&actor,WF_A_Y,0x01000000u);
  wf_physics_integrate_common(&actor);
  int32_t dx=(int32_t)wf_be32(&actor,WF_A_X)-0x01000000;
  int32_t dy=(int32_t)wf_be32(&actor,WF_A_Y)-0x01000000;
  unsigned ok=dx==desired[i][0]&&dy==desired[i][1];errors+=!ok;
  printf("%s %s %s physical_low=%02x ROM_F2D6_index=%u angle=%04x delta_fixed=(%d,%d) desired=(%d,%d)\n",ok?"PASS":"FAIL",argv[2],names[i],low,index,angle,dx,dy,desired[i][0],desired[i][1]);
 }
 return errors?1:0;
}
