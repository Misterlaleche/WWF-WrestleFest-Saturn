#include "wf_r18_runtime_handoff.h"
#include "wf_r17_bootstrap.h"
#include <string.h>
wf_status_t wf_r18_scene_resume(const wf_runtime_t*r,uint8_t*raw,size_t bytes,
 const uint8_t*saved,size_t saved_bytes,wf_r18_video_t*v,const wf_r18_world_ctx_t*w,
 const wf_r18_sprite_record_t*records,size_t count,const uint16_t queues[6]){
 if(!r||!r->m68k_regs.valid||!raw||bytes<0x4000u||!saved||saved_bytes<0x4000u||
    !v||!w||(!records&&count)||count>512u||!queues)return WF_BAD_STATE;
 for(unsigned i=0;i<6;i++)if(queues[i])return WF_BLOCKED;
 if(w->animation_count[0]||w->animation_count[1])return WF_BLOCKED;
 memmove(raw,saved,0x4000u);
 memcpy(raw+0x1600,v->palette_used,sizeof v->palette_used);
 memcpy(raw+0x1610,v->palette_map,sizeof v->palette_map);
 /* The palette map overlaps named gameplay globals. Export them last;
  * original world cache LONGs in turn own their overlap with $16E6. */
 (void)wf_r18_runtime_export(r,raw,bytes);
 for(unsigned side=0;side<2;side++)for(unsigned i=0;i<16;i++){
  uint32_t at=0x16d6u+side*128u+i*8u;
  uint32_t source=w->cache[side][i].source,dest=w->cache[side][i].destination;
  for(unsigned b=0;b<4;b++){
   raw[at+b]=(uint8_t)(source>>(24u-b*8u));
   raw[at+4u+b]=(uint8_t)(dest>>(24u-b*8u));
  }
 }
 raw[0x19b2]=raw[0x19b4]=(uint8_t)(count>>8);
 raw[0x19b3]=raw[0x19b5]=(uint8_t)count;
 for(unsigned i=0;i<6;i++)raw[0x19b8+i*4u]=raw[0x19b9+i*4u]=0;
 if(count)memmove(v->fg0+0x2000,records,count*sizeof *records);
 return WF_OK;
}
static uint16_t r16(const uint8_t*p){return (uint16_t)((p[0]<<8)|p[1]);}
static uint32_t r32(const uint8_t*p){return ((uint32_t)r16(p)<<16)|r16(p+2);}
static void w16(uint8_t*p,uint16_t v){p[0]=(uint8_t)(v>>8);p[1]=(uint8_t)v;}
static void w32(uint8_t*p,uint32_t v){w16(p,(uint16_t)(v>>16));w16(p+2,(uint16_t)v);}
wf_status_t wf_r18_runtime_import(wf_runtime_t*r,const uint8_t*raw,size_t bytes,const wf_r17_m68k_shadow_t*regs){
 if(!r||!raw||bytes<0x4000u||!regs||!regs->valid)return WF_BAD_STATE;
 memset(&r->g,0,sizeof r->g);
 /* Literal addresses and widths declared by wf_globals_raw_t. No selection,
  * initial health, credit or seed policy is applied during this transfer. */
 r->g.credit_cycle_units_004b=raw[0x004b];
 r->g.credit_remainder_004e=raw[0x004e];
 r->g.credit_count_004f=raw[0x004f];
 r->g.config_0066=r16(raw+0x0066);
 r->g.gameplay_vblank_enable=raw[0x0076];
 r->g.global_0063=raw[0x0063];
 r->g.global_0072=r16(raw+0x0072);
 r->g.credited_route=r16(raw+0x007c);
 r->g.global_0092=r32(raw+0x0092);
 r->g.global_015e=r16(raw+0x015e);
 r->g.global_0164=r16(raw+0x0164);
 r->g.frame_counter=r32(raw+0x0080);
 r->g.global_0082=raw[0x0082];
 r->g.pipeline_mode=raw[0x0083];
 r->g.mode_0160=raw[0x0160];
 r->g.mode_0161=raw[0x0161];
 r->g.progression_0162=r16(raw+0x0162);
 r->g.global_0166=r16(raw+0x0166);
 r->g.global_1682=r16(raw+0x1682);
 r->g.global_1684=r32(raw+0x1684);
 r->g.global_1688=r32(raw+0x1688);
 r->g.global_168c=r32(raw+0x168c);
 r->g.global_1692=r16(raw+0x1692);
 r->g.global_1690=r16(raw+0x1690);
 r->g.global_1698=r16(raw+0x1698);
 r->g.global_1694=r16(raw+0x1694);
 r->g.global_1697=raw[0x1697];
 r->g.global_169a=r16(raw+0x169a);
 r->g.global_169b=raw[0x169b];
 r->g.global_169e=r16(raw+0x169e);
 r->g.global_16a0=r16(raw+0x16a0);
 r->g.global_16a2=r16(raw+0x16a2);
 r->g.global_16a4=r16(raw+0x16a4);
 r->g.global_16a7=raw[0x16a7];
 r->g.global_16c4=r16(raw+0x16c4);
 r->g.global_16ca=r16(raw+0x16ca);
 r->g.global_16cc=r16(raw+0x16cc);
 r->g.global_16ce=r16(raw+0x16ce);
 r->g.global_15ce=r16(raw+0x15ce);
 r->g.global_1678=raw[0x1678];
 r->g.global_1679=raw[0x1679];
 r->g.global_167a=raw[0x167a];
 r->g.global_167b=raw[0x167b];
 r->g.global_1670=r32(raw+0x1670);
 r->g.global_1674=r32(raw+0x1674);
 r->g.global_007e=r16(raw+0x007e);
 r->g.global_007f=raw[0x007f];
 r->g.global_16d2=r16(raw+0x16d2);
 r->g.global_16d4=r16(raw+0x16d4);
 r->g.global_17d6=r16(raw+0x17d6);
 r->g.global_17d8=r16(raw+0x17d8);
 r->g.global_17da=r16(raw+0x17da);
 r->g.global_17dc=r16(raw+0x17dc);
 r->g.global_17de=r16(raw+0x17de);
 r->g.global_17e0=r16(raw+0x17e0);
 r->g.global_17e2=r16(raw+0x17e2);
 r->g.global_17e4=r16(raw+0x17e4);
 r->g.global_17e6=r16(raw+0x17e6);
 r->g.global_17ea=r16(raw+0x17ea);
 r->g.global_17ee=r16(raw+0x17ee);
 r->g.global_17f2_word=r16(raw+0x17f2);
 r->g.global_17f6=r16(raw+0x17f6);
 r->g.global_17fa=r16(raw+0x17fa);
 r->g.global_17fe=r16(raw+0x17fe);
 r->g.global_1800=r16(raw+0x1800);
 r->g.global_1802=r16(raw+0x1802);
 r->g.global_1150=r16(raw+0x1150);
 r->g.global_1180=r16(raw+0x1180);
 r->g.global_11b0=r16(raw+0x11b0);
 r->g.global_11e0=r16(raw+0x11e0);
 r->g.global_1804=r16(raw+0x1804);
 r->g.global_1806=r16(raw+0x1806);
 r->g.transfer_count_1828=r16(raw+0x1828);
 r->g.global_15f4=r16(raw+0x15f4);
 r->g.global_15fc=r16(raw+0x15fc);
 r->g.global_15f8=r16(raw+0x15f8);
 r->g.global_15d2=raw[0x15d2];
 r->g.global_15d3=raw[0x15d3];
 r->g.global_15d4=raw[0x15d4];
 r->g.global_15d5=raw[0x15d5];
 r->g.global_15d6=r16(raw+0x15d6);
 r->g.global_15d8=r16(raw+0x15d8);
 r->g.global_00aa=r16(raw+0x00aa);
 r->g.global_00b2=r16(raw+0x00b2);
 r->g.global_00a3=raw[0x00a3];
 r->g.cabinet_edge_15da=raw[0x15da];
 r->g.cabinet_level_15db=raw[0x15db];
 r->g.cabinet_edge_15dc=raw[0x15dc];
 r->g.cabinet_level_15dd=raw[0x15dd];
 r->g.global_16e6=raw[0x16e6];
 r->g.rng_state=r32(raw+0x005c);
 memcpy(r->g.frame_handshake,raw+0x6e,2);
 for(unsigned i=0;i<4;i++)r->g.global_0156[i]=r16(raw+0x156+i*2u);
 for(unsigned i=0;i<12;i++)r->g.royal_occupancy[i]=r16(raw+0x16a8+i*2u);
 for(unsigned i=0;i<10;i++)r->g.royal_slots_1586[i]=r16(raw+0x1586+i*2u);
 memcpy(r->g.royal_selector_1576,raw+0x1576,sizeof r->g.royal_selector_1576);
 memcpy(r->g.transfer_queue_1808,raw+0x1808,sizeof r->g.transfer_queue_1808);
 memcpy(r->g.direction_scratch_15e4,raw+0x15e4,sizeof r->g.direction_scratch_15e4);
 memcpy(r->g.cabinet_tag_15e0,raw+0x15e0,sizeof r->g.cabinet_tag_15e0);
 r->g.rng_state_valid=r->g.rng_register_context_valid=1;
 r->g.rng_d2=regs->d[2];r->g.rng_d3=regs->d[3];r->g.rng_d4=regs->d[4];
 r->g.rng_d5=regs->d[5];r->g.rng_d6=regs->d[6];r->g.rng_d7=regs->d[7];
 memcpy(r->actors,raw+0x5b0,sizeof r->actors);
 memcpy(r->fixed_object0.b,raw+0xf1c,sizeof r->fixed_object0.b);
 memcpy(r->fixed_object1.b,raw+0x1028,sizeof r->fixed_object1.b);
 wf_actor_raw_t *fixed[5]={&r->fixed_four_1134,&r->referee_aux,&r->fixed_four_1194,&r->fixed_four_11c4,&r->referee};
 static const uint16_t fixed_at[5]={0x1134,0x1164,0x1194,0x11c4,0x11f4};
 for(unsigned i=0;i<5;i++){memset(fixed[i],0,sizeof *fixed[i]);memcpy(fixed[i]->b,raw+fixed_at[i],i==4?0x64u:0x30u);}
 memcpy(r->transient_1258,raw+0x1258,sizeof r->transient_1258);
 memcpy(r->branchb_list_182a,raw+0x182a,sizeof r->branchb_list_182a);
 memcpy(r->branchb_prev_185a,raw+0x185a,sizeof r->branchb_prev_185a);
 memcpy(r->branchb_objs_188a,raw+0x188a,sizeof r->branchb_objs_188a);
 for(unsigned i=0;i<3;i++){memset(&r->effect_1450[i],0,sizeof r->effect_1450[i]);memcpy(r->effect_1450[i].b,raw+0x1450+i*0x2au,0x2a);}
 for(unsigned i=0;i<4;i++){memset(&r->compact_14ce[i],0,sizeof r->compact_14ce[i]);memcpy(r->compact_14ce[i].b,raw+0x14ceu+i*0x2au,0x2a);}
 r->m68k_regs=*regs;return WF_OK;
}
wf_status_t wf_r18_runtime_handoff_f8c(wf_runtime_t*r,uint8_t*raw,size_t bytes,wf_r17_m68k_shadow_t*regs){
 if(!r||!raw||bytes<0x4000u||!regs||!regs->valid)return WF_BAD_STATE;
 wf_r17_f8c_state_t s={.control_1c0076=r16(raw+0x76),.sync_1c006e=r16(raw+0x6e),.counter_1c0080=r32(raw+0x80),.branch_1c0083=raw[0x83]};
 uint32_t seed=r32(raw+0x5c);wf_status_t st=wf_r17_f8c_enter(&s,&seed,regs,NULL);if(st!=WF_OK)return st;
 w16(raw+0x76,s.control_1c0076);w16(raw+0x6e,s.sync_1c006e);w32(raw+0x80,s.counter_1c0080);w32(raw+0x5c,seed);
 /* Import after CLR.L80 so byte82/83 follow the real shared-RAM alias. */
 st=wf_r18_runtime_import(r,raw,bytes,regs);if(st!=WF_OK)return st;
 r->frame_ready=r->vblank_ack=0;r->frame_stage=3;return WF_OK;
}

/* Publish the typed view back into borrowed arcade RAM. Unrepresented bytes
 * stay with the caller. Overlapping selector/fixed records are copied first;
 * the named mutable WORD/LONG owners then supply their canonical aliases. */
wf_status_t wf_r18_runtime_export(const wf_runtime_t*r,uint8_t*raw,size_t bytes){
 if(!r||!raw||bytes<0x4000u||!r->m68k_regs.valid)return WF_BAD_STATE;
 memcpy(raw+0x5b0,r->actors,sizeof r->actors);
 memcpy(raw+0xf1c,r->fixed_object0.b,sizeof r->fixed_object0.b);
 memcpy(raw+0x1028,r->fixed_object1.b,sizeof r->fixed_object1.b);
 const wf_actor_raw_t *fixed[5]={&r->fixed_four_1134,&r->referee_aux,&r->fixed_four_1194,&r->fixed_four_11c4,&r->referee};
 static const uint16_t fixed_at[5]={0x1134,0x1164,0x1194,0x11c4,0x11f4};
 for(unsigned i=0;i<5;i++)memcpy(raw+fixed_at[i],fixed[i]->b,i==4?0x64u:0x30u);
 memcpy(raw+0x1258,r->transient_1258,sizeof r->transient_1258);
 memcpy(raw+0x182a,r->branchb_list_182a,sizeof r->branchb_list_182a);
 memcpy(raw+0x185a,r->branchb_prev_185a,sizeof r->branchb_prev_185a);
 memcpy(raw+0x188a,r->branchb_objs_188a,sizeof r->branchb_objs_188a);
 for(unsigned i=0;i<3;i++)memcpy(raw+0x1450+i*0x2au,r->effect_1450[i].b,0x2au);
 for(unsigned i=0;i<4;i++)memcpy(raw+0x14ceu+i*0x2au,r->compact_14ce[i].b,0x2au);
 memcpy(raw+0x1576,r->g.royal_selector_1576,sizeof r->g.royal_selector_1576);
 raw[0x004b]=r->g.credit_cycle_units_004b;
 raw[0x004e]=r->g.credit_remainder_004e;
 raw[0x004f]=r->g.credit_count_004f;
 w16(raw+0x0066,r->g.config_0066);
 raw[0x0076]=r->g.gameplay_vblank_enable;
 raw[0x0063]=r->g.global_0063;
 w16(raw+0x0072,r->g.global_0072);
 w16(raw+0x007c,r->g.credited_route);
 w32(raw+0x0092,r->g.global_0092);
 w16(raw+0x015e,r->g.global_015e);
 w16(raw+0x0164,r->g.global_0164);
 w32(raw+0x0080,r->g.frame_counter);
 raw[0x0160]=r->g.mode_0160;
 raw[0x0161]=r->g.mode_0161;
 w16(raw+0x0162,r->g.progression_0162);
 w16(raw+0x0166,r->g.global_0166);
 w16(raw+0x1682,r->g.global_1682);
 w32(raw+0x1684,r->g.global_1684);
 w32(raw+0x1688,r->g.global_1688);
 w32(raw+0x168c,r->g.global_168c);
 w16(raw+0x1692,r->g.global_1692);
 w16(raw+0x1690,r->g.global_1690);
 w16(raw+0x1698,r->g.global_1698);
 w16(raw+0x1694,r->g.global_1694);
 raw[0x1697]=r->g.global_1697;
 w16(raw+0x169a,r->g.global_169a);
 w16(raw+0x169e,r->g.global_169e);
 w16(raw+0x16a0,r->g.global_16a0);
 w16(raw+0x16a2,r->g.global_16a2);
 w16(raw+0x16a4,r->g.global_16a4);
 raw[0x16a7]=r->g.global_16a7;
 w16(raw+0x16c4,r->g.global_16c4);
 w16(raw+0x16ca,r->g.global_16ca);
 w16(raw+0x16cc,r->g.global_16cc);
 w16(raw+0x16ce,r->g.global_16ce);
 w16(raw+0x15ce,r->g.global_15ce);
 raw[0x1678]=r->g.global_1678;
 raw[0x1679]=r->g.global_1679;
 raw[0x167a]=r->g.global_167a;
 raw[0x167b]=r->g.global_167b;
 w32(raw+0x1670,r->g.global_1670);
 w32(raw+0x1674,r->g.global_1674);
 w16(raw+0x007e,r->g.global_007e);
 w16(raw+0x16d2,r->g.global_16d2);
 w16(raw+0x16d4,r->g.global_16d4);
 w16(raw+0x17d6,r->g.global_17d6);
 w16(raw+0x17d8,r->g.global_17d8);
 w16(raw+0x17da,r->g.global_17da);
 w16(raw+0x17dc,r->g.global_17dc);
 w16(raw+0x17de,r->g.global_17de);
 w16(raw+0x17e0,r->g.global_17e0);
 w16(raw+0x17e2,r->g.global_17e2);
 w16(raw+0x17e4,r->g.global_17e4);
 w16(raw+0x17e6,r->g.global_17e6);
 w16(raw+0x17ea,r->g.global_17ea);
 w16(raw+0x17ee,r->g.global_17ee);
 w16(raw+0x17f2,r->g.global_17f2_word);
 w16(raw+0x17f6,r->g.global_17f6);
 w16(raw+0x17fa,r->g.global_17fa);
 w16(raw+0x17fe,r->g.global_17fe);
 w16(raw+0x1800,r->g.global_1800);
 w16(raw+0x1802,r->g.global_1802);
 w16(raw+0x1150,r->g.global_1150);
 w16(raw+0x1180,r->g.global_1180);
 w16(raw+0x11b0,r->g.global_11b0);
 w16(raw+0x11e0,r->g.global_11e0);
 w16(raw+0x1804,r->g.global_1804);
 w16(raw+0x1806,r->g.global_1806);
 w16(raw+0x1828,r->g.transfer_count_1828);
 w16(raw+0x15f4,r->g.global_15f4);
 w16(raw+0x15fc,r->g.global_15fc);
 w16(raw+0x15f8,r->g.global_15f8);
 raw[0x15d2]=r->g.global_15d2;
 raw[0x15d3]=r->g.global_15d3;
 raw[0x15d4]=r->g.global_15d4;
 raw[0x15d5]=r->g.global_15d5;
 w16(raw+0x15d6,r->g.global_15d6);
 w16(raw+0x15d8,r->g.global_15d8);
 w16(raw+0x00aa,r->g.global_00aa);
 w16(raw+0x00b2,r->g.global_00b2);
 raw[0x00a3]=r->g.global_00a3;
 raw[0x15da]=r->g.cabinet_edge_15da;
 raw[0x15db]=r->g.cabinet_level_15db;
 raw[0x15dc]=r->g.cabinet_edge_15dc;
 raw[0x15dd]=r->g.cabinet_level_15dd;
 raw[0x16e6]=r->g.global_16e6;
 w32(raw+0x005c,r->g.rng_state);
 memcpy(raw+0x6e,r->g.frame_handshake,2);
 for(unsigned i=0;i<4;i++)w16(raw+0x156+i*2u,r->g.global_0156[i]);
 for(unsigned i=0;i<12;i++)w16(raw+0x16a8+i*2u,r->g.royal_occupancy[i]);
 for(unsigned i=0;i<10;i++)w16(raw+0x1586+i*2u,r->g.royal_slots_1586[i]);
 memcpy(raw+0x1808,r->g.transfer_queue_1808,sizeof r->g.transfer_queue_1808);
 memcpy(raw+0x15e4,r->g.direction_scratch_15e4,sizeof r->g.direction_scratch_15e4);
 memcpy(raw+0x15e0,r->g.cabinet_tag_15e0,sizeof r->g.cabinet_tag_15e0);
 return WF_OK;
}
