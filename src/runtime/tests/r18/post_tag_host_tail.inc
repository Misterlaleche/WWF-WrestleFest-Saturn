/* Appended only to a frozen HOST probe. Production sources are unchanged. */
#include "wf_r18_post_tag.h"
static unsigned post_boot_boundary,post_ranking_calls;
static wf_status_t post_ram_read(wf_runtime_t*r,uint32_t at,uint16_t*out,void*u){
 (void)r;(void)u;if(at<0x1c0000u||at>0x1c3ffeu)return WF_BLOCKED;
 *out=(uint16_t)read_be(h.ram+at-0x1c0000u,2);return WF_OK;
}
static wf_status_t post_owner(uint32_t owner,wf_r18_prematch_t*m){
 if(owner==0x206fe)return wf_r18_referee_clear_206fe(&h.runtime,&h.video);
 if(owner==0x2067c)return wf_r18_referee_number_2067c(&h.runtime,(uint16_t)m->regs->d[0],&h.video);
 if(owner!=0x11b6&&owner!=0x122c&&owner!=0xb608&&owner!=0xbda6&&owner!=0xae20&&owner!=0x15f4&&owner!=0x139c&&owner!=0x160e){
  if(owner==0x65c0)printf("POST OWNER %06x progress=%u seed=%08x\n",owner,h.ram[0x163],read_be(h.ram+0x5c,4));
  wf_status_t st=wf_r18_match_driver_owner(&combat_paid,owner);
  if(owner==0x65c0&&st==WF_OK)post_ranking_calls++;
  return st;
 }
 wf_r18_post_tag_t s;wf_r18_post_tag_init(&s,owner);wf_r18_post_tag_context_t c={m,rom,&h};
 for(unsigned iterations=0;iterations<1000000u;iterations++){
  h.driver.pc=s.pc;int status=wf_r18_post_tag_step(&s,&c);
  if(status==WF_R18_POST_AC0||status==WF_R18_POST_RETURN)return WF_OK;
  if(status==WF_R18_POST_BOUNDARY&&s.pc==0x6fc&&getenv("WF_HUMAN_FOLLOW_LOSS")){post_boot_boundary=s.pc;return WF_OK;}
  if(status==WF_R18_POST_MORE)continue;
  if(status!=WF_R18_POST_REQUEST){fprintf(stderr,"POST block pc=%06x dep=%06x status%d\n",s.pc,s.blocked_dependency,status);return WF_BLOCKED;}
  wf_status_t st=WF_BLOCKED;uint16_t returned_value=0;
  if(s.request.kind==WF_R18_POST_CALL)st=post_owner(s.request.owner,m);
  else if(s.request.kind==WF_R18_POST_WRITE16){
   if(s.request.address>=0x180000u&&s.request.address<0x188800u){wf_r17_fg0_write_t w={s.request.address,s.request.value,2};st=wf_r18_video_batch(&h.runtime,&w,1,&h.video);}
   else st=store_bus(&h,s.request.address,s.request.value,2);
  }else if(s.request.kind==WF_R18_POST_SYNC){
   memcpy(h.driver.a,m->a,sizeof m->a);st=h.driver.platform.wait_vblank(&h,&h.driver);memcpy(m->a,h.driver.a,sizeof m->a);
  }else if(s.request.kind==WF_R18_POST_READ16||s.request.kind==WF_R18_POST_READ8){
   uint32_t value;st=read_bus(&h,s.request.address&~1u,&value,2);
   if(st==WF_OK)returned_value=s.request.kind==WF_R18_POST_READ16?(uint16_t)value:(uint16_t)((value>>((s.request.address&1u)?0:8))&255u);
  }
  if(wf_r18_post_tag_complete(&s,&c,st,returned_value)<0){fprintf(stderr,"POST dependency pc=%06x owner=%06x dep=%06x\n",s.pc,s.request.owner,s.blocked_dependency);return WF_BLOCKED;}
 }
 return WF_BLOCKED;
}
static wf_status_t post_bind(void*u,unsigned mode){wf_status_t st=bind(u,mode);if(st==WF_OK)h.video.ring_cell=NULL;return st;}
static int post_loss_resume(void){
 assert(post_boot_boundary==0x6fc&&post_ranking_calls==1u);
 for(unsigned i=0;i<4;i++)assert(!(wf_u8(&h.runtime.actors[i],WF_A_RESULT)&0x40u));
 assert(!h.coin&&!h.start);
 uint8_t raw_before[sizeof h.ram],records[0x84];
 memcpy(raw_before,h.ram,sizeof h.ram);memcpy(records,h.ram+0xc2,sizeof records);
 wf_r17_m68k_shadow_t regs=h.driver.regs;uint32_t ar[7];memcpy(ar,h.driver.a,sizeof ar);
 uint16_t sr=h.driver.control.sr;uint32_t ssp=h.driver.control.ssp,usp=h.driver.control.usp;
 unsigned generation=h.driver.generation,before=h.frames;
 combat_dump("loss-before-6fc-ram.bin",h.ram,sizeof h.ram);
 assert(wf_r18_frontend_driver_resume_boot(&h.driver,0x6fc)==WF_OK);
 assert(h.driver.phase==WF_R18_DRIVER_BOOT&&h.driver.pc==0x6fc&&h.driver.generation==generation+1u);
 assert(!memcmp(raw_before,h.ram,sizeof h.ram)&&!memcmp(&regs,&h.driver.regs,sizeof regs)&&!memcmp(ar,h.driver.a,sizeof ar));
 assert(h.driver.control.sr==sr&&h.driver.control.ssp==ssp&&h.driver.control.usp==usp);
 for(unsigned i=0;h.driver.phase!=WF_R18_DRIVER_TITLE;i++){
  assert(i<200000u);
  if(wf_r18_frontend_driver_step(&h.driver)<0){fprintf(stderr,"LOSS REENTRY block pc=%06x dep=%06x\n",h.driver.pc,h.driver.blocked_dependency);return 1;}
 }
 assert(!memcmp(records,h.ram+0xc2,sizeof records));
 combat_dump("loss-return-title-ram.bin",h.ram,sizeof h.ram);
 combat_dump("loss-return-title-fg.bin",h.video.fg0,sizeof h.video.fg0);
 combat_dump("loss-return-title-background.bin",h.video.background,sizeof h.video.background);
 combat_dump("loss-return-title-palettes.bin",h.video.palettes,sizeof h.video.palettes);
 printf("LOSS RETURN PASS boundary=%06x phase=%u pc=%06x bootframes=%u records=preserved ranking_calls=%u\n",post_boot_boundary,h.driver.phase,h.driver.pc,h.frames-before,post_ranking_calls);
 return 0;
}
int main(int argc,char**argv){
 int status=post_tag_combat_main(argc,argv);fflush(stdout);setvbuf(stdout,NULL,_IONBF,0);if(status)return status;
 assert(wf_r18_tag_match_complete(&h.runtime));
 /* HOST keeps the full original raw buffer alive throughout combat. Typed
  * owners are published over it, along with the authoritative video state. */
 size_t cap,post_tag_last_record_count=wf_r18_present_records_count();wf_r18_sprite_record_t*records=wf_r18_present_records_buffer(&cap);assert(post_tag_last_record_count<=cap);
 assert(wf_r18_scene_resume(&h.runtime,h.ram,sizeof h.ram,h.ram,sizeof h.ram,&h.video,&combat_world,records,post_tag_last_record_count,combat_gc.render_queue.count)==WF_OK);
 for(unsigned i=0;i<4;i++)write_be(h.display+i*2u,combat_display.scroll[i],2);
 h.board[0x10]=combat_priority;
 h.driver.regs=h.runtime.m68k_regs;h.driver.phase=WF_R18_DRIVER_AC0;
 h.video.read_rom=rom;h.video.read_ram=post_ram_read;h.video.read_user=&h;h.driver.platform.bind_world=post_bind;
 combat_dump("post-11b6-ram.bin",h.ram,sizeof h.ram);combat_dump("post-11b6-dregs.bin",h.driver.regs.d,sizeof h.driver.regs.d);
 unsigned before=h.frames;assert(post_owner(0x11b6,&combat_paid.machine)==WF_OK);
 memcpy(h.driver.a,combat_paid.machine.a,sizeof h.driver.a);h.driver.pc=0xac0;
 if(post_boot_boundary)return post_loss_resume();
 combat_dump("post-ac0-ram.bin",h.ram,sizeof h.ram);
 printf("POST TAG AC0 frames=%u progress=%u previous=%u world=%u records=%zu\n",h.frames-before,h.ram[0x163],h.ram[0x165],read_be(h.ram+0x7e,2),post_tag_last_record_count);
 assert(h.ram[0x163]==1u);
 assert(wf_r18_match_driver_init(&combat_paid,&h.driver,combat_extension,NULL)==WF_OK);
 status=wf_r18_match_driver_run(&combat_paid);
 printf("NEXT MATCH status=%u pc=%06x dep=%06x flow=%06x world=%u seed=%08x frames=%u\n",status,combat_paid.blocked_pc,combat_paid.blocked_dependency,combat_paid.flow.pc,read_be(h.ram+0x7e,2),read_be(h.ram+0x5c,4),h.frames-before);
 assert(status==WF_OK&&combat_paid.flow.pc==0xf8c&&read_be(h.ram+0x7e,2)==5u);
 combat_dump("next-f8c-ram.bin",h.ram,sizeof h.ram);return 0;
}
