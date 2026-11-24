#include "wf_r18_match_driver.h"
#include "wf_r18_match_aux.h"
#include "wf_r18_match_hud.h"
#include "wf_r18_match_flash.h"
#include "wf_r18_card_scene.h"
#include "wf_r18_attract_setup.h"
#include "wf_r18_post_tag.h"
#include "wf_r18_ranking.h"
#include "wf_r18_ranking_glyph.h"
#include "wf_r18_records_aux.h"
#include <string.h>
static uint16_t rw(const uint8_t*p){return (uint16_t)((p[0]<<8)|p[1]);}
static void to_front(wf_r18_match_driver_t*c){memcpy(c->frontend->a,c->machine.a,sizeof c->machine.a);}
static void from_front(wf_r18_match_driver_t*c){memcpy(c->machine.a,c->frontend->a,sizeof c->machine.a);}
static wf_status_t failed(wf_r18_match_driver_t*c,uint32_t owner,uint32_t pc){
 if(c->generation!=c->frontend->generation)return WF_BLOCKED;
 if(!c->blocked_dependency){c->blocked_dependency=c->machine.dependency?c->machine.dependency:owner;c->blocked_pc=pc;}
 return WF_BLOCKED;
}
static wf_status_t palette(void*u,uint32_t at,uint16_t value){
 wf_r18_match_driver_t*c=u;wf_r17_fg0_write_t w={at,value,2};
 return wf_r18_video_batch(c->frontend->runtime,&w,1,c->frontend->video);
}
static wf_status_t rom(void*u,uint32_t at,uint8_t*p,size_t n){
 wf_r18_frontend_driver_t*d=((wf_r18_match_driver_t*)u)->frontend;return d->platform.read_rom(d->platform.user,at,p,n);
}
static wf_status_t io(void*u,unsigned kind,uint32_t pc,uint32_t at,uint16_t*value,wf_r18_prematch_t*m){
 wf_r18_match_driver_t*c=u;wf_r18_frontend_driver_t*d=c->frontend;(void)m;to_front(c);d->pc=pc;
 wf_status_t st;
 if(kind==WF_R18_PREMATCH_SYNC)st=d->platform.wait_vblank(d->platform.user,d);
 else if(kind==WF_R18_PREMATCH_READ16){uint32_t v;st=d->platform.read_bus(d->platform.user,at,&v,2);if(st==WF_OK)*value=(uint16_t)v;}
 else st=WF_BLOCKED;
 from_front(c);
 if(c->generation!=d->generation)return WF_BLOCKED;
 return st==WF_OK?WF_OK:failed(c,pc,pc);
}
static wf_status_t effect(void*u,uint32_t owner,wf_r18_prematch_t*m){
 wf_r18_match_driver_t*c=u;(void)m;return wf_r18_match_driver_owner(c,owner);
}
static wf_status_t post_rom(void*u,uint32_t at,void*out,size_t n){return rom(u,at,out,n);}
static wf_status_t ranking_store(void*u,uint32_t at,uint32_t value,unsigned width){
 wf_r18_match_driver_t*c=u;wf_r18_frontend_driver_t*d=c->frontend;at&=0xffffffu;
 if((at>=0xc0000u&&at<0xc4000u)||(at>=0x80000u&&at<0x90000u)||
    (at>=0x180000u&&at<0x184000u)||(at>=0x186000u&&at<0x186800u)||(at>=0x188000u&&at<0x188800u)){
  wf_r17_fg0_write_t w={at,value,(uint8_t)width};return wf_r18_video_batch(d->runtime,&w,1,d->video);
 }
 return d->platform.store_bus(d->platform.user,at,value,width);
}
static wf_status_t ranking_read(wf_r18_match_driver_t*c,uint32_t at,uint32_t*out,unsigned width){
 wf_r18_frontend_driver_t*d=c->frontend;at&=0xffffffu;
 if((width==1u||width==2u||width==4u)&&at>=0xc0000u&&at<=0xc4000u-width){
  *out=0;for(unsigned i=0;i<width;i++)*out=(*out<<8)|d->video->fg0[at-0xc0000u+i];return WF_OK;
 }
 to_front(c);wf_status_t st=d->platform.read_bus(d->platform.user,at,out,width);from_front(c);return st;
}
static wf_status_t ranking(wf_r18_match_driver_t*c,uint32_t owner){
 wf_r18_frontend_driver_t*d=c->frontend;wf_r18_ranking_t s;wf_r18_ranking_init(&s,owner);
 wf_r18_ranking_context_t x={&c->machine,post_rom,c};
 for(;;){
  d->pc=s.pc;int r=wf_r18_ranking_step(&s,&x);
  if(r==WF_R18_RANK_RETURN)return WF_OK;
  if(r==WF_R18_RANK_MORE)continue;
  if(r!=WF_R18_RANK_REQUEST)return failed(c,s.blocked_dependency?s.blocked_dependency:s.pc,s.pc);
  wf_status_t st=WF_BLOCKED;uint32_t value=0;
  if(s.request.kind==WF_R18_RANK_CALL)st=wf_r18_match_driver_owner(c,s.request.owner);
  else if(s.request.kind==WF_R18_RANK_WRITE)st=ranking_store(c,s.request.address,s.request.value,s.request.width);
  else if(s.request.kind==WF_R18_RANK_READ)st=ranking_read(c,s.request.address,&value,s.request.width);
  else if(s.request.kind==WF_R18_RANK_SYNC){uint16_t v=0;st=io(c,WF_R18_PREMATCH_SYNC,s.pc,0,&v,&c->machine);}
  if(c->generation!=d->generation)return WF_BLOCKED;
  if(wf_r18_ranking_complete(&s,&x,st,value)<0)return failed(c,s.blocked_dependency?s.blocked_dependency:s.pc,s.pc);
 }
}
static wf_status_t post_ae20(wf_r18_match_driver_t*c){
 wf_r18_frontend_driver_t*d=c->frontend;
 wf_r18_post_tag_t s;wf_r18_post_tag_init(&s,0xae20u);
 wf_r18_post_tag_context_t x={&c->machine,post_rom,c};
 for(;;){
  d->pc=s.pc;int r=wf_r18_post_tag_step(&s,&x);
  if(r==WF_R18_POST_RETURN)return WF_OK;
  if(r==WF_R18_POST_MORE)continue;
  if(r!=WF_R18_POST_REQUEST)return failed(c,s.blocked_dependency?s.blocked_dependency:s.pc,s.pc);
  wf_status_t st=WF_BLOCKED;uint16_t value=0;
  if(s.request.kind==WF_R18_POST_CALL)st=wf_r18_match_driver_owner(c,s.request.owner);
  else if(s.request.kind==WF_R18_POST_SYNC)st=io(c,WF_R18_PREMATCH_SYNC,s.pc,0,&value,&c->machine);
  else if(s.request.kind==WF_R18_POST_WRITE16){
   uint32_t at=s.request.address;
   if((at>=0x180000u&&at<0x184000u)||(at>=0x186000u&&at<0x186800u)||(at>=0x188000u&&at<0x188800u)){
    wf_r17_fg0_write_t w={at,s.request.value,2};st=wf_r18_video_batch(d->runtime,&w,1,d->video);
   }else st=d->platform.store_bus(d->platform.user,at,s.request.value,2);
  }else if(s.request.kind==WF_R18_POST_READ16||s.request.kind==WF_R18_POST_READ8){
   uint32_t v;to_front(c);
   st=d->platform.read_bus(d->platform.user,s.request.address,&v,s.request.kind==WF_R18_POST_READ8?1u:2u);
   from_front(c);if(st==WF_OK)value=(uint16_t)v;
  }
  if(c->generation!=d->generation)return WF_BLOCKED;
  if(wf_r18_post_tag_complete(&s,&x,st,value)<0)return failed(c,s.blocked_dependency?s.blocked_dependency:s.pc,s.pc);
 }
}
wf_status_t wf_r18_match_driver_init(wf_r18_match_driver_t*c,wf_r18_frontend_driver_t*d,wf_r18_match_extension_fn extension,void*u){
 if(!c||!d||!d->ram||d->phase!=WF_R18_DRIVER_AC0)return WF_BAD_STATE;
 memset(c,0,sizeof *c);c->frontend=d;c->extension=extension;c->user=u;
 c->generation=d->generation;
 c->machine=(wf_r18_prematch_t){.ram=d->ram,.regs=&d->regs,.effect=effect,.user=c};from_front(c);
 wf_r18_match_flow_init(&c->flow,0xac0);return WF_OK;
}
wf_status_t wf_r18_match_driver_owner(wf_r18_match_driver_t*c,uint32_t owner){
 if(!c||!c->frontend||c->blocked_dependency||c->generation!=c->frontend->generation)return WF_BLOCKED;
 wf_r18_frontend_driver_t*d=c->frontend;wf_status_t st=WF_BLOCKED;uint32_t pc=d->pc;
 switch(owner){
 case 0x65c0:case 0x69c8:case 0x6a26:st=ranking(c,owner);break;
 case 0x6cfa:case 0x6d76:{
  wf_r18_ranking_glyph_context_t g={&c->machine,ranking_store,c,0,0};
  st=wf_r18_ranking_glyph_call(&g,owner);break;
 }
 case 0xa4d4:case 0x7874:{
  wf_r18_records_aux_context_t g={&c->machine,post_rom,c,ranking_store,0,0};
  st=wf_r18_records_aux_call(&g,owner);break;
 }
 case 0xae20:st=post_ae20(c);break;
 case 0xb1c:{
  wf_r18_attract_setup_t s;wf_r18_attract_setup_init(&s,owner);int r;
  for(;;){
   d->pc=s.pc;r=wf_r18_attract_setup_step(&s,&c->machine);
   if(r==WF_R18_ATTRACT_RETURN){st=WF_OK;break;}
   if(r==WF_R18_ATTRACT_MORE)continue;
   if(r!=WF_R18_ATTRACT_REQUEST)return failed(c,s.blocked_dependency,s.pc);
   uint16_t value=0;
   if(s.request.kind==WF_R18_ATTRACT_CALL)st=wf_r18_match_driver_owner(c,s.request.owner);
   else if(s.request.kind==WF_R18_ATTRACT_READ16)st=io(c,WF_R18_PREMATCH_READ16,s.pc,s.request.address,&value,&c->machine);
   else st=WF_BLOCKED;
   if(c->generation!=d->generation)return WF_BLOCKED;
   r=wf_r18_attract_setup_complete(&s,&c->machine,st,value);
   if(r<0)return failed(c,s.blocked_dependency,s.pc);
   if(r==WF_R18_ATTRACT_WAIT){
    to_front(c);st=d->platform.wait_level(d->platform.user,d,s.pc==0x21eau);from_front(c);
    if(st!=WF_OK||c->generation!=d->generation)return failed(c,owner,s.pc);
   }
  }break;
 }
 case 0x1086c:st=wf_r18_match_setup_1086c(&c->machine);break;
 case 0x8490:to_front(c);st=wf_r18_frontend_driver_attract_visual_8490(d);from_front(c);break;
 case 0xc98:{
  wf_r18_match_join_t s;wf_r18_match_join_init(&s);int r;
  do{d->pc=s.pc;r=wf_r18_match_join_step(&s,&c->machine);}while(r==WF_R18_PREMATCH_MORE);
  if(r==WF_R18_PREMATCH_DONE)st=WF_OK;else return failed(c,s.blocked_dependency,s.pc);break;
 }
 case 0xa654:{
  wf_r18_prematch_scene_t s;wf_r18_prematch_scene_init(&s);int r;
  do{d->pc=s.pc;r=wf_r18_prematch_scene_step(&s,&c->machine,io,c);}while(r==WF_R18_PREMATCH_MORE||r==WF_R18_PREMATCH_WAIT);
  if(r==WF_R18_PREMATCH_DONE)st=WF_OK;else return failed(c,s.blocked_dependency,s.pc);break;
 }
 case 0x7b70:{
  wf_r18_card_scene_t s;wf_r18_card_scene_init(&s,owner);int r;
  for(;;){
   d->pc=s.pc;r=wf_r18_card_scene_step(&s,&c->machine);
   if(r==WF_R18_CARD_RETURN){st=WF_OK;break;}
   if(r==WF_R18_CARD_MORE)continue;
   if(r!=WF_R18_CARD_REQUEST)return failed(c,s.blocked_dependency,s.pc);
   if(s.request.kind==WF_R18_CARD_CALL)st=wf_r18_match_driver_owner(c,s.request.owner);
   else if(s.request.kind==WF_R18_CARD_WRITE8)st=d->platform.store_bus(d->platform.user,s.request.address,s.request.value,1);
   else if(s.request.kind==WF_R18_CARD_SYNC){uint16_t v=2;st=io(c,WF_R18_PREMATCH_SYNC,s.pc,s.request.address,&v,&c->machine);}
   else st=WF_BLOCKED;
   r=wf_r18_card_scene_complete(&s,&c->machine,st);
   if(r<0)return failed(c,s.blocked_dependency,s.pc);
  }break;
 }
 case 0x8f50:case 0x8fd6:{
  wf_r18_match_flash_t s;wf_r18_match_flash_init(&s,owner);
  wf_r18_match_flash_context_t f={&c->machine,d->video->fg0,sizeof d->video->fg0,io,c};int r;
  do{
   d->pc=s.pc;r=wf_r18_match_flash_step(&s,&f);
   if(s.pc==0x8f7eu||s.pc==0x9004u){
    memset(d->video->hud_dirty,0xff,sizeof d->video->hud_dirty);
    if(d->platform.submit_records(d->platform.user,(const void*)(d->video->fg0+0x2000),rw(d->ram+0x19b2))!=WF_OK)return failed(c,owner,s.pc);
   }
   /* READ16 is the original busy poll. Give the transport the exact
    * requested physical level only while the cursor remains at that poll. */
   if(r==WF_R18_PREMATCH_WAIT&&(s.pc==0x21eau||s.pc==0x21f4u)){
    to_front(c);st=d->platform.wait_level(d->platform.user,d,s.pc==0x21eau);from_front(c);
    if(st!=WF_OK||c->generation!=d->generation)return failed(c,owner,s.pc);
   }
  }while(r==WF_R18_PREMATCH_MORE||r==WF_R18_PREMATCH_WAIT);
  if(r==WF_R18_PREMATCH_DONE)st=WF_OK;else return failed(c,s.blocked_dependency,s.pc);break;
 }
 case 0x7506:case 0x782c:case 0xc1ea:{
  wf_r18_match_hud_t h={.machine=&c->machine,.fg_0c0000=d->video->fg0,.fg_bytes=sizeof d->video->fg0,.read_rom=rom,.user=c,.write_palette=palette};
  st=wf_r18_match_hud_call(&h,owner);memset(d->video->hud_dirty,0xff,sizeof d->video->hud_dirty);break;
 }
 case 0x2208:case 0xb29e:case 0xb2e4:st=wf_r18_match_aux_call(&c->machine,d->video,owner);break;
 case 0x98ba:case 0x8eb6:case 0x285da:
  if(c->extension)st=c->extension(c->user,owner,&c->machine);
  break;
 default:
  to_front(c);st=wf_r18_frontend_driver_call(d,owner);from_front(c);break;
 }
 if(c->generation!=d->generation)return WF_BLOCKED;
 if(st!=WF_OK)return failed(c,owner,d->pc);
 d->pc=pc;return WF_OK;
}
wf_status_t wf_r18_match_driver_run(wf_r18_match_driver_t*c){
 if(!c||!c->frontend)return WF_BAD_STATE;
 for(;;){
  c->frontend->pc=c->flow.pc;int r=wf_r18_match_flow_step(&c->flow,&c->machine);
  if(r==WF_R18_MATCH_GAMEPLAY){to_front(c);return WF_OK;}
  if(r==WF_R18_MATCH_MORE)continue;
  if(r!=WF_R18_MATCH_REQUEST)return failed(c,c->flow.blocked_dependency,c->flow.pc);
  if(c->flow.request.kind!=WF_R18_MATCH_CALL)return failed(c,c->flow.request.owner,c->flow.pc);
  wf_status_t st=wf_r18_match_driver_owner(c,c->flow.request.owner);
  if(c->generation!=c->frontend->generation)return WF_BLOCKED;
  if(wf_r18_match_flow_complete(&c->flow,&c->machine,st,0)<0)return failed(c,c->flow.request.owner,c->flow.pc);
 }
}
