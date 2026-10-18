#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "wf_gameplay_bindings.h"
#include "wf_r17_m68k_shadow.h"
#include "wf_r17_rng1129e.h"
int main(int argc,char **argv){
 wf_runtime_t r;wf_gameplay_binding_context_t c;wf_phase_binding_t b[40];
 size_t n=wf_gameplay_bindings_build(&c,b,40);wf_runtime_init(&r,b,n);
 c.camera_valid=1;c.camera.scroll0_x=0x140;c.camera.scroll0_y=0x200;
 r.g.rng_state_valid=1;r.g.rng_state=0x13579bdf;
 for(unsigned j=0;j<8;j++)r.m68k_regs.d[j]=0x11110000+j*0x1111;
 wf_actor_raw_t *a=&r.actors[0];wf_set_u8(a,WF_A_ACTIVE,0x80);
 wf_set_be16(a,WF_A_STATE,0x8005);wf_set_be16(a,WF_A_LATCHED,0x8005);
 wf_set_be16(a,WF_A_SELECTOR,0x7c);wf_set_be16(a,WF_A_ANIM_INDEX,0xfe);
 wf_set_be32(a,WF_A_LINK,wf_actor_rom_base(1));
 wf_set_be16(a,WF_A_X,0x280);wf_set_be16(a,WF_A_Y,0x150);wf_set_be16(a,WF_A_Z,0x100);
 assert(wf_phase_wrestlers_f4c2(&r,0xf4c2,&c)==WF_OK);
 if(argc>1&&strcmp(argv[1],"--dump")==0){
  for(unsigned j=0;j<8;j++){printf("%08x ",r.m68k_regs.d[j]);}printf("%08x\n",r.g.rng_state);
  for(unsigned k=0;k<2;k++){for(unsigned j=0;j<WF_ACTOR_BYTES;j++)printf("%02x",r.actors[k].b[j]);puts("");}
 }
 assert(r.m68k_regs.d[0]==0x11110009);assert(r.m68k_regs.d[1]==0x11110000);assert(r.m68k_regs.d[2]==0x11110000);
 for(unsigned j=3;j<8;j++)assert(r.m68k_regs.d[j]==0x11110000+j*0x1111);
 assert(wf_be16(a,WF_A_X)==0x260);assert(wf_be16(a,WF_A_SELECTOR)==0x17);
 r.m68k_regs.d[0]=wf_r17_rng21b4_step(&r.g.rng_state,0,r.m68k_regs.d);
 assert(r.g.rng_state==0x8ad04688);assert(r.m68k_regs.d[0]==0x8ad04688);
 puts("PASS active $F4C2 raw $7C terminal callback: ROM-derived D0-D7, movement and following RNG");return 0;
}
