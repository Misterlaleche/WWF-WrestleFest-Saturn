#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "wf_r17_contract.h"
#include "wf_gameplay_bindings.h"
#include "wf_r17_referee.h"
#include "wf_r17_fixed_four_10120.h"

static wf_status_t input_word(wf_runtime_t*rt,uint32_t address,uint16_t*value,void*user){
 (void)rt;(void)user;assert(address>=0x140020u&&address<=0x140026u&&!(address&1u));*value=0xffffu;return WF_OK;
}
static unsigned char rom[0x100000];static size_t rom_size;
static uint16_t rom16(size_t p){assert(p+2<=rom_size);return (uint16_t)((rom[p]<<8)|rom[p+1]);}
static uint32_t rom32(size_t p){return ((uint32_t)rom16(p)<<16)|rom16(p+2);}
static wf_status_t render_ok(wf_runtime_t *rt,wf_actor_raw_t *const *actors,size_t count,void *user){
 (void)user;assert(count==4u); /* referee, moving wrestler, two fixed actors */
 for(size_t i=0;i<count;i++){
  wf_actor_raw_t*a=actors[i];
  if(wf_u8(a,WF_A_STATE+1u)==0xffu||(wf_be16(a,WF_A_MIRROR)&0x7fffu)==0x7fffu)continue;
  uint16_t ch=wf_be16(a,WF_A_CHARACTER),pose=wf_be16(a,WF_A_MIRROR);
  uint16_t table_offset=rom16(0x38f14u+(uint16_t)(ch<<1));assert(table_offset!=0);
  size_t poses=0x38f14u+table_offset;
  assert(rom16(poses+(uint16_t)(pose<<1))!=0xfffeu);
  size_t record=rom32(0x38fb8u+(uint16_t)(ch<<2))+rom16(poses+(uint16_t)(pose<<1));
  assert(record<rom_size);
  wf_status_t s=wf_r17_render_record_shadow(rt,a,rom+record,rom_size-record);if(s!=WF_OK)return s;
 }
 return WF_OK;
}


static wf_phase_binding_t orig[40];
static wf_status_t probe(wf_runtime_t*r,uint32_t pc,void*user){

 wf_status_t s=WF_BAD_STATE;for(unsigned j=0;j<40;j++)if(orig[j].rom_addr==pc){s=orig[j].fn(r,pc,user);break;}
 printf("%06x %u",pc,s);for(unsigned j=0;j<8;j++)printf(" %08x",r->m68k_regs.d[j]);printf(" %08x\n",r->g.rng_state);return s;
}
int main(int argc,char **argv){
    assert(argc==3);
    setvbuf(stdout,NULL,_IOLBF,0);
    FILE *rom_file=fopen(argv[2],"rb");assert(rom_file);rom_size=fread(rom,1,sizeof(rom),rom_file);assert(rom_size>0x40000);fclose(rom_file);
    wf_runtime_t rt;
    wf_phase_binding_t b[40];
    wf_gameplay_binding_context_t c;
    size_t n=wf_gameplay_bindings_build(&c,b,40u);

    assert(n>=29u);
    wf_runtime_init(&rt,b,n);

    rt.g.rng_state_valid=1u;
    rt.g.rng_state=0x13579bdfu;
    rt.g.pipeline_mode=0u;              /* WP28 branch A */
    rt.g.credited_route=1u;             /* normal credited route */
    rt.g.mode_0161=0u;                  /* non-Royal post-ACK route */
    rt.g.global_169e=0x3000u;           /* exact early gates for $262D2/$9052 */
    rt.g.global_167b=0x01u;             /* exact $899C early return */

    c.cabinet_words_valid=1u;
    c.cabinet_word_140020=0xffffu;
    c.cabinet_word_140022=0xffffu;
    c.camera_valid=1u;
    /* Same canonical RAM as the independent ROM fixture. Camera phases
     * import these words; the standalone adapter is not their owner. */
    rt.g.global_17e6=rt.g.global_17ea=0x0140;
    rt.g.global_17ee=rt.g.global_17f2_word=0x0200;
    c.camera.scene_index=0u;
    c.camera.scroll0_x=0x0140;
    c.camera.scroll1_x=0x0140;
    c.camera.scroll0_y=0x0200;
    c.camera.scroll1_y=0x0200;
    c.post7_render_2836_batch=render_ok;
    c.participant_input_word=input_word;


    /* Two active wrestlers for the referee. Actor0 is changed below into
     * a genuine, visible raw7C terminal action; actor1 remains a passive target. */
    for(unsigned i=0u;i<2u;i++){
        wf_set_u8(&rt.actors[i],WF_A_ACTIVE,0x80u);
        wf_set_u8(&rt.actors[i],WF_A_FLAGS32,0x80u);
        wf_set_u8(&rt.actors[i],WF_A_FLAGS33,0x01u);
        wf_set_u8(&rt.actors[i],WF_A_STATE+1u,0x07u);
        wf_set_be16(&rt.actors[i],WF_A_X,(uint16_t)(0x0200u+i*0x20u));
        wf_set_be16(&rt.actors[i],WF_A_Y,0x0180u);
        c.input_valid_mask=(uint16_t)(c.input_valid_mask|(1u<<i));
        c.input_hw_low[i]=0xffu;
    }
    wf_set_be32(&rt.actors[0],WF_A_INPUT_SOURCE,0x140020);wf_set_be32(&rt.actors[1],WF_A_INPUT_SOURCE,0x140022);
    wf_set_u8(&rt.actors[0],WF_A_FLAGS32,0);
    wf_set_be16(&rt.actors[0],WF_A_STATE,0x8005);wf_set_be16(&rt.actors[0],WF_A_LATCHED,0x8005);
    wf_set_be16(&rt.actors[0],WF_A_SELECTOR,0x7c);wf_set_be16(&rt.actors[0],WF_A_ANIM_INDEX,0xfe);
    wf_set_be32(&rt.actors[0],WF_A_LINK,wf_actor_rom_base(1));
    wf_set_be16(&rt.actors[0],WF_A_Z,0x140u);
    wf_r17_referee_init_10718(&rt);
    wf_r17_fixed_four_init_1004c(&rt);

    FILE *ram=fopen(argv[1],"wb");
    unsigned char mem[0x2000]={0};
    memcpy(mem+0x5b0,rt.actors,sizeof(rt.actors));
    memcpy(mem+0x11f4,rt.referee.b,0x64);
    memcpy(mem+0x1134,rt.fixed_four_1134.b,0x30);memcpy(mem+0x1164,rt.referee_aux.b,0x30);
    memcpy(mem+0x1194,rt.fixed_four_1194.b,0x30);memcpy(mem+0x11c4,rt.fixed_four_11c4.b,0x30);
    mem[0x7d]=1;mem[0x169e]=0x30;mem[0x167b]=1;mem[0x1697]=1;
    mem[0x5c]=0x13;mem[0x5d]=0x57;mem[0x5e]=0x9b;mem[0x5f]=0xdf;
    mem[0x17e6]=1;mem[0x17e7]=0x40;mem[0x17ea]=1;mem[0x17eb]=0x40;
    mem[0x17ee]=2;mem[0x17f2]=2;
    assert(ram);assert(fwrite(mem,1,sizeof(mem),ram)==sizeof(mem));fclose(ram);
    for(size_t j=0;j<n;j++){orig[j]=b[j];b[j].fn=probe;}
    wf_status_t result=wf_runtime_frame(&rt);
    printf("RESULT %u TRACE %zu DEP %x\n",result,rt.trace_count,c.last_blocked_dependency);
    assert(result==WF_BLOCKED&&rt.trace_count==29u&&rt.frame_ready==1u);
    assert(wf_be16(&rt.actors[0],WF_A_X)==0x1e0u); /* raw7C actual -32 movement */
    assert(wf_be16(&rt.actors[0],WF_A_SELECTOR)==0x17u);
    wf_runtime_vblank_ack(&rt);assert(wf_runtime_frame(&rt)==WF_OK);
    assert(rt.g.frame_counter==1u&&rt.g.pipeline_mode==1u&&rt.g.global_0082==0u);
    printf("ACK");for(unsigned j=0;j<8;j++)printf(" %08x",rt.m68k_regs.d[j]);printf(" %08x\n",rt.g.rng_state);
    return 0;
}
