#include "wf_r18_runtime_handoff.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static uint8_t saved[0x4000],raw[0x4000],before[0x4000];
static wf_runtime_t runtime;
static wf_r18_video_t video,video_before;
static wf_r18_world_ctx_t world;
static wf_r18_sprite_record_t records[512];
static uint16_t queues[6];
static unsigned r16(const uint8_t*p){return (unsigned)p[0]<<8|p[1];}
static uint32_t r32(const uint8_t*p){return (uint32_t)r16(p)<<16|r16(p+2);}
int main(void){
 wf_r17_m68k_shadow_t regs={.valid=1};
 for(unsigned n=0;n<64;n++){
  for(unsigned i=0;i<sizeof saved;i++)saved[i]=(uint8_t)(i*37u+n*23u);
  assert(wf_r18_runtime_import(&runtime,saved,sizeof saved,&regs)==WF_OK);
  runtime.g.global_1670=0x1234u;runtime.g.rng_state=0xdeadbeefu+n;
  runtime.g.credit_count_004f=7;runtime.g.frame_counter=0x789abcdeu;
  memset(video.palette_map,0xa5,sizeof video.palette_map);
  memset(video.palette_used,0x3c,sizeof video.palette_used);
  memset(video.fg0+0x2000,0xd7,0x2000);memset(records,0x69,sizeof records);
  for(unsigned side=0;side<2;side++)for(unsigned i=0;i<16;i++){
   world.cache[side][i].source=0x345600u+side*0x100+i*8;
   world.cache[side][i].destination=0x80000u+side*0x2000+i*4;
  }
  assert(wf_r18_scene_resume(&runtime,raw,sizeof raw,saved,sizeof saved,&video,&world,records,n,queues)==WF_OK);
  assert(raw[0x4f]==7&&r32(raw+0x5c)==0xdeadbeefu+n);
  assert(r32(raw+0x80)==0x789abcdeu&&r32(raw+0x1670)==0x1234u);
  assert(raw[0x1600]==0x3c&&raw[0x1610]==0xa5);
  assert(!memcmp(raw+0xc2,saved+0xc2,0x94));
  assert(!memcmp(raw+0x168,saved+0x168,0x438));
  assert(!memcmp(raw+0x1cd2,saved+0x1cd2,sizeof saved-0x1cd2));
  for(unsigned side=0;side<2;side++)for(unsigned i=0;i<16;i++){
   assert(r32(raw+0x16d6+side*128+i*8)==world.cache[side][i].source);
   assert(r32(raw+0x16da+side*128+i*8)==world.cache[side][i].destination);
  }
  assert(r16(raw+0x19b2)==n&&r16(raw+0x19b4)==n);
  for(unsigned i=0;i<6;i++)assert(r16(raw+0x19b8+i*4)==0);
  assert(!memcmp(video.fg0+0x2000,records,n*16u));
  assert(video.fg0[0x2000+n*16u]==0xd7);
 }
 memcpy(before,raw,sizeof raw);video_before=video;
 queues[3]=1;
 assert(wf_r18_scene_resume(&runtime,raw,sizeof raw,saved,sizeof saved,&video,&world,records,1,queues)==WF_BLOCKED);
 queues[3]=0;world.animation_count[0]=1;
 assert(wf_r18_scene_resume(&runtime,raw,sizeof raw,saved,sizeof saved,&video,&world,records,1,queues)==WF_BLOCKED);
 world.animation_count[0]=0;
 assert(wf_r18_scene_resume(&runtime,raw,sizeof raw,saved,sizeof saved-1,&video,&world,records,1,queues)==WF_BAD_STATE);
 assert(wf_r18_scene_resume(&runtime,raw,sizeof raw,saved,sizeof saved,&video,&world,records,513,queues)==WF_BAD_STATE);
 runtime.m68k_regs.valid=0;
 assert(wf_r18_scene_resume(&runtime,raw,sizeof raw,saved,sizeof saved,&video,&world,records,1,queues)==WF_BAD_STATE);
 assert(!memcmp(raw,before,sizeof raw)&&!memcmp(&video,&video_before,sizeof video));
 puts("PASS scene resume:64 retained full-RAM images, alias precedence, world caches, exact record prefixes and atomic guards");
 return 0;
}
