/* Validation-only controller. The included, frozen frontend/probe supplies
 * all boot, paid handoff, engine, IRQ and result callbacks unchanged.
 * Only normal active-low input levels are replaced before a new frame. */
#define wf_runtime_frame human_tag_runtime_frame
#define wf_r18_result_in_ring human_tag_result_in_ring
#include "full_flow_combat_probe.c"
#undef wf_runtime_frame
#undef wf_r18_result_in_ring

extern wf_status_t wf_runtime_frame(wf_runtime_t *);
extern wf_status_t wf_r18_result_in_ring(wf_runtime_t *,wf_r18_video_t *,
 wf_r18_result_wait_fn,wf_r18_result_commit_fn,void *);
static unsigned strategy,period=4,on=2,reach=12,tolerance=4,bias,mash=1;
static int configured;
static uint32_t last_input_frame=0xffffffffu;
static FILE *human_tape;
static unsigned fresh_samples,pin_samples;
static const wf_actor_raw_t *human_target(const wf_runtime_t*r,const wf_actor_raw_t*a){
 const wf_actor_raw_t*t=NULL;uint32_t raw=wf_be32(a,WF_A_TARGET)&0xffffffu;
 if(raw>=0x1c05b0u&&(raw-0x1c05b0u)%0x10cu==0u){unsigned i=(raw-0x1c05b0u)/0x10cu;if(i<9u)t=&r->actors[i];}
 if(strategy!=1u){
  const wf_actor_raw_t*b=&r->actors[4],*c=&r->actors[5];
  int br=(wf_u8(b,0)&128u)&&((wf_u8(b,0x33)&5u)==1u);
  int cr=(wf_u8(c,0)&128u)&&((wf_u8(c,0x33)&5u)==1u);
  if(br||cr)t=!br?c:!cr?b:wf_be16(b,0x66)<=wf_be16(c,0x66)?b:c;
  else if(!t)t=b;
 }
 return t?t:&r->actors[4];
}
static uint8_t human_pad(const wf_runtime_t*r){
 const wf_actor_raw_t*a=&r->actors[0],*t=human_target(r,a);
 unsigned state=wf_u8(a,0x21),sel=wf_u8(a,0x61),ts=wf_u8(t,0x21),sub=wf_u8(t,0x65);
 int ground=ts==4u&&(sub==6u||sub==7u||sub==8u||sub==9u);
 int dx=(int16_t)(wf_be16(t,6)-wf_be16(a,6)),dy=(int16_t)(wf_be16(t,10)-wf_be16(a,10));
 uint8_t bits=0xffu;unsigned frame=r->g.frame_counter,pulse=frame%period<on;
 int goalx=dx;
 if(ground&&bias)goalx+=(wf_u8(t,0x2e)&128u)?(int)bias:-(int)bias;
 if(state==0u||state==1u||state==2u){
  if(goalx>(int)reach)bits&=(uint8_t)~1u;else if(goalx<-(int)reach)bits&=(uint8_t)~2u;
  if(dy>(int)tolerance)bits&=(uint8_t)~4u;else if(dy<-(int)tolerance)bits&=(uint8_t)~8u;
 }
 if(pulse){
  uint8_t press=0;
  if(state==4u||state==6u||(state==5u&&(sel==0x4au||sel==0x7bu||sel==0x52u||sel==0x53u)))press=mash?0x70u:0x30u;
  else if(state==5u&&(sel==0x15u||sel==0x16u))press=((frame/period)&1u)?0x10u:0x20u;
  else if(dx>-104&&dx<104&&dy>-32&&dy<24){
   if(ground){press=0x20u;pin_samples++;}
   else if(strategy==2u)press=0x30u;
   else if(strategy==3u)press=0x20u;
   else if(strategy==4u)press=((frame/period)&1u)?0x10u:0x20u;
   else if(strategy==5u)press=(frame/period)%3u==0u?0x30u:0x10u;
   else press=0x10u;
  }
  bits&=(uint8_t)~press;if(press)fresh_samples++;
 }
 return bits;
}
wf_status_t human_tag_runtime_frame(wf_runtime_t*r){
 if(!configured){
  const char*s=getenv("WF_HUMAN_POLICY");if(s)assert(sscanf(s,"%u,%u,%u,%u,%u,%u,%u",&strategy,&period,&on,&reach,&tolerance,&bias,&mash)==7);
  assert(period>=4u&&on>=2u&&period-on>=2u&&strategy<6u&&reach<=64u&&tolerance<=24u&&bias<=64u);
  assert(!getenv("WF_CAPTURED_HANDOFF_RAM"));
  char path[4096];snprintf(path,sizeof path,"%s/human-input.csv",combat_output);human_tape=fopen(path,"w");assert(human_tape);
  fputs("frame,pad,hp0,hp1,hp4,hp5,state0,sel0,targetstate,targetsub,targethp,flags0,flags4,flags5\n",human_tape);
  printf("HUMAN_POLICY %u,%u,%u,%u,%u,%u,%u; input levels only; win requires original BYTEA0.bit6\n",strategy,period,on,reach,tolerance,bias,mash);configured=1;
 }
 if(r->g.frame_counter!=last_input_frame){
  wf_runtime_t before=*r;uint8_t pad=human_pad(r);assert(!memcmp(&before,r,sizeof before));
  combat_gc.input_hw_low[0]=pad;last_input_frame=r->g.frame_counter;
  const wf_actor_raw_t*t=human_target(r,&r->actors[0]);
  fprintf(human_tape,"%u,%02x,%u,%u,%u,%u,%02x,%04x,%02x,%02x,%u,%02x,%02x,%02x\n",r->g.frame_counter,pad,wf_be16(&r->actors[0],0x66),wf_be16(&r->actors[1],0x66),wf_be16(&r->actors[4],0x66),wf_be16(&r->actors[5],0x66),wf_u8(&r->actors[0],0x21),wf_be16(&r->actors[0],0x60),wf_u8(t,0x21),wf_u8(t,0x65),wf_be16(t,0x66),wf_u8(&r->actors[0],0x33),wf_u8(&r->actors[4],0x33),wf_u8(&r->actors[5],0x33));
 }
 return wf_runtime_frame(r);
}
wf_status_t human_tag_result_in_ring(wf_runtime_t*r,wf_r18_video_t*v,
 wf_r18_result_wait_fn wait,wf_r18_result_commit_fn commit,void*u){
 unsigned winners=0;
 for(unsigned i=0;i<4u;i++)if(wf_u8(&r->actors[i],WF_A_RESULT)&0x40u)winners|=1u<<i;
 printf("HUMAN_TAG_OUTCOME %s frame=%u winners=%x result=%04x,%04x,%04x,%04x role=%04x,%04x,%04x,%04x input_samples=%u pin_samples=%u\n",winners?"WIN":"LOSS",r->g.frame_counter,winners,wf_be16(&r->actors[0],0xa0),wf_be16(&r->actors[1],0xa0),wf_be16(&r->actors[2],0xa0),wf_be16(&r->actors[3],0xa0),wf_be16(&r->actors[0],0xfe),wf_be16(&r->actors[1],0xfe),wf_be16(&r->actors[2],0xfe),wf_be16(&r->actors[3],0xfe),fresh_samples,pin_samples);
 return wf_r18_result_in_ring(r,v,wait,commit,u);
}

int human_tag_next_combat(unsigned frames){
 assert(frames>0u&&frames<=20000u&&combat_paid.flow.pc==0xf8cu);
 wf_phase_binding_t*bindings=h.runtime.bindings;size_t count=h.runtime.binding_count;
 uint32_t seed_before=read_be(h.ram+0x5c,4);
 assert(wf_r18_runtime_handoff_f8c(&h.runtime,h.ram,sizeof h.ram,&h.driver.regs)==WF_OK);
 assert(h.runtime.bindings==bindings&&h.runtime.binding_count==count);
 assert(h.runtime.frame_stage==3u&&h.runtime.g.frame_counter==0u&&h.runtime.g.global_007e==5u);
 combat_dump("second-handoff-ram.bin",h.ram,sizeof h.ram);
 combat_dump("second-handoff-runtime.bin",&h.runtime,sizeof h.runtime);
 wf_runtime_t before=h.runtime;
 uint32_t bytes;assert(!combat_scene_size("R18MAP5.BIN",&bytes)&&bytes<=sizeof combat_ring);
 h.video.ring_cell=NULL;(void)wf_r18_ring_bind(NULL,0);
 assert(!combat_scene_read("R18MAP5.BIN",0,combat_ring,bytes));
 assert(read_be(combat_ring+6,2)==5u&&read_be(combat_ring+8,2)==5u);
 assert(!wf_r18_ring_bind(combat_ring,bytes));
 wf_r18_present_init(combat_file,combat_rom_cache,sizeof combat_rom_cache,combat_tile_cache,sizeof combat_tile_cache);
 assert(wf_r18_rom_pages_preload(wf_r18_rom_hot_pages,sizeof wf_r18_rom_hot_pages/sizeof wf_r18_rom_hot_pages[0])==WF_OK);
 assert(wf_r18_hot_sprites_load()==WF_OK);assert(!memcmp(&before,&h.runtime,sizeof before));
 for(unsigned side=0;side<2;side++)for(unsigned i=0;i<16;i++){
  const uint8_t*p=h.ram+0x16d6+side*128u+i*8u;
  combat_world.cache[side][i]=(wf_r18_world_transport_t){read_be(p,4),read_be(p+4,4)};
 }
 combat_world.animation_count[0]=combat_world.animation_count[1]=0;
 combat_setup();combat_gc.camera.scene_index=h.runtime.g.global_007e;
 last_input_frame=0xffffffffu;
 printf("SECOND HANDOFF stage=%u world=%u camera=%u seed-before=%08x seed-after=%08x picks=%u,%u,%u,%u bindings=%zu mapbytes=%u\n",h.runtime.frame_stage,h.runtime.g.global_007e,combat_gc.camera.scene_index,seed_before,h.runtime.g.rng_state,wf_u8(&h.runtime.actors[0],0x57),wf_u8(&h.runtime.actors[1],0x57),wf_u8(&h.runtime.actors[4],0x57),wf_u8(&h.runtime.actors[5],0x57),count,bytes);
 for(unsigned i=0;i<frames;i++){
  wf_status_t st=human_tag_runtime_frame(&h.runtime);
  if(st==WF_BLOCKED&&h.runtime.frame_ready){
   st=wf_r18_display_irq(&h.runtime,&combat_display);
   if(st==WF_OK)st=wf_r18_world_commit(&h.runtime,&combat_world);
   if(st==WF_OK)st=wf_r18_hud_irq(&h.runtime,&h.video);
   if(st==WF_OK){wf_runtime_vblank_ack(&h.runtime);st=human_tag_runtime_frame(&h.runtime);}
  }
  if(st!=WF_OK){combat_block(i,st);return 1;}
  assert(h.runtime.g.global_007e==5u&&combat_gc.camera.scene_index==5u);
 }
 combat_dump("second-final-runtime.bin",&h.runtime,sizeof h.runtime);
 combat_dump("second-final-actors.bin",h.runtime.actors,sizeof h.runtime.actors);
 combat_dump("second-final-fg.bin",h.video.fg0,sizeof h.video.fg0);
 combat_dump("second-final-background.bin",h.video.background,sizeof h.video.background);
 combat_dump("second-final-palettes.bin",h.video.palettes,sizeof h.video.palettes);
 printf("SECOND COMBAT PASS frames=%u runtime_frame=%u world=%u camera=%u rng=%08x hp=%u,%u,%u,%u\n",frames,h.runtime.g.frame_counter,h.runtime.g.global_007e,combat_gc.camera.scene_index,h.runtime.g.rng_state,wf_be16(&h.runtime.actors[0],0x66),wf_be16(&h.runtime.actors[1],0x66),wf_be16(&h.runtime.actors[4],0x66),wf_be16(&h.runtime.actors[5],0x66));
 return 0;
}
