#include "wf_r18_frontend_driver.h"
#include "wf_r18_frontend_palette.h"
#include "wf_r18_title_palette.h"
#include <string.h>
static uint16_t rw(const uint8_t*p){return (uint16_t)((p[0]<<8)|p[1]);}
static void ww(uint8_t*p,uint16_t v){p[0]=(uint8_t)(v>>8);p[1]=(uint8_t)v;}
static wf_status_t fatal(wf_r18_frontend_driver_t*d,uint32_t pc,uint32_t at){
 if(d->phase!=WF_R18_DRIVER_FATAL){d->blocked_dependency=pc;d->blocked_address=at;d->phase=WF_R18_DRIVER_FATAL;}
 return WF_BLOCKED;
}
static wf_status_t read_rom(void*u,uint32_t at,void*p,size_t n){
 wf_r18_frontend_driver_t*d=u;return d->platform.read_rom(d->platform.user,at,p,n);
}
static wf_status_t read_ram(wf_runtime_t*r,uint32_t at,uint16_t*out,void*u){
 wf_r18_frontend_driver_t*d=u;(void)r;at&=0xffffffu;
 if(!out||at<0x1c0000u||at>0x1c3ffeu)return WF_BLOCKED;
 *out=rw(d->ram+at-0x1c0000u);return WF_OK;
}
static wf_status_t read_bus(void*u,uint32_t at,uint32_t*v,unsigned n){
 wf_r18_frontend_driver_t*d=u;return d->platform.read_bus(d->platform.user,at&0xffffffu,v,n);
}
static int video_address(uint32_t at,unsigned n){
 return (at>=0xc0000u&&at+n<=0xc4000u)||(at>=0x80000u&&at+n<=0x84000u)||
  (at>=0x180000u&&at+n<=0x184000u)||(at>=0x186000u&&at+n<=0x186800u)||
  (at>=0x188000u&&at+n<=0x188800u);
}
static wf_status_t store_bus(void*u,uint32_t at,uint32_t value,unsigned n){
 wf_r18_frontend_driver_t*d=u;at&=0xffffffu;
 if(video_address(at,n)){wf_r17_fg0_write_t w={at,value,(uint8_t)n};return wf_r18_video_batch(d->runtime,&w,1,d->video);}
 return d->platform.store_bus(d->platform.user,at,value,n);
}
static wf_status_t video_batch(wf_runtime_t*r,const wf_r17_fg0_write_t*w,size_t n,void*u){
 wf_r18_frontend_driver_t*d=u;return wf_r18_video_batch(r,w,n,d->video);
}
static wf_status_t text(wf_runtime_t*r,uint32_t pc,void*u){
 wf_r18_frontend_driver_t*d=u;
 if(pc==0x2503c)return wf_r18_text_2503c(r,(uint16_t)r->m68k_regs.d[0],d->video);
 if(pc==0x26122)return wf_r18_number_26122(r,(uint16_t)r->m68k_regs.d[0],d->video);
 return WF_BLOCKED;
}
static wf_status_t wait_level(wf_r18_frontend_driver_t*d,unsigned high,uint32_t pc){
 uint32_t generation=d->generation,old_pc=d->pc,value;d->pc=pc;
 if(read_bus(d,0x140026,&value,2)!=WF_OK)return fatal(d,pc,0x140026);
 if(!!(value&0x400u)!=!!high){
  wf_status_t st=d->platform.wait_level(d->platform.user,d,high);
  if(d->generation!=generation)return WF_BLOCKED;
  if(st!=WF_OK)return fatal(d,pc,0x140026);
  /* The platform wait is synchronous: it has already observed this real
   * level. Re-reading MMIO here could miss a short VBlank after a slow
   * controller/audio transaction and restart a completed instruction. */
 }
 d->pc=old_pc;return WF_OK;
}
static wf_status_t wait_vblank(wf_r18_frontend_driver_t*d,uint32_t pc){
 uint32_t generation=d->generation,old_pc=d->pc;d->pc=pc;
 wf_status_t st=d->platform.wait_vblank(d->platform.user,d);
 if(d->generation!=generation)return WF_BLOCKED;
 if(st!=WF_OK)return fatal(d,pc,0x1c006f);
 d->pc=old_pc;return WF_OK;
}
static uint8_t *palette(wf_r18_frontend_driver_t*d,unsigned role){
 if(role==0)return d->video->palettes;
 if(role==1)return d->video->palettes+0x2000;
 return role==2?d->video->bg_palette:d->video->fg_palette;
}
static wf_status_t read_palette(void*u,unsigned role,uint16_t colors[256]){
 wf_r18_frontend_driver_t*d=u;const uint8_t*p=palette(d,role);
 for(unsigned i=0;i<256;i++)colors[i]=rw(p+(i/16u)*128u+(i%16u)*2u);
 return WF_OK;
}
static wf_status_t write_palette(void*u,unsigned role,uint16_t colors[256]){
 static const uint32_t bases[4]={0x180000,0x182000,0x186000,0x188000};
 for(unsigned i=0;i<256;i++){
  wf_status_t st=store_bus(u,bases[role]+(i/16u)*128u+(i%16u)*2u,colors[i],2);if(st!=WF_OK)return st;
 }
 return WF_OK;
}
static wf_status_t fade(wf_r18_frontend_driver_t*d,uint32_t owner){
 d->fade.ram=d->ram;d->fade.regs=&d->regs;d->fade.read_palette=read_palette;d->fade.write_palette=write_palette;d->fade.user=d;
 if(wf_r18_frontend_effect_begin(&d->fade,owner)!=WF_OK)return WF_BLOCKED;
 while(d->fade.active){
  unsigned high=d->fade.step&1u;
  if(wait_level(d,high,owner)!=WF_OK)return WF_BLOCKED;
  if(wf_r18_frontend_effect_resume(&d->fade,high)==WF_R18_FRONTEND_EFFECT_ERROR)return WF_BLOCKED;
 }
 return WF_OK;
}
static wf_status_t ports(wf_r18_frontend_driver_t*d,uint16_t values[4]){
 for(unsigned i=0;i<4;i++){uint32_t v;wf_status_t st=read_bus(d,0x140020u+i*2u,&v,2);if(st!=WF_OK)return fatal(d,d->pc,0x140020u+i*2u);values[i]=(uint16_t)v;}
 return WF_OK;
}
static int title_effect(void*u,const wf_r18_title_event_t*e,uint8_t*ram,wf_r17_m68k_shadow_t*r,uint32_t a[7]){
 wf_r18_frontend_driver_t*d=u;(void)ram;(void)r;(void)a;
 if(e->kind==WF_R18_TITLE_CALL)return wf_r18_frontend_driver_call(d,e->owner_rom)!=WF_OK;
 if(e->kind==WF_R18_TITLE_SCROLL16){wf_status_t st=store_bus(d,e->address,e->value,2);if(st!=WF_OK)fatal(d,e->owner_rom,e->address);return st!=WF_OK;}
 if(e->kind==WF_R18_TITLE_SYNC)return wait_vblank(d,e->owner_rom)!=WF_OK;
 return fatal(d,e->owner_rom,e->address)!=WF_OK;
}
static int control_effect(void*u,wf_r18_title_event_t*e,wf_r18_frontend_control_context_t*c){
 wf_r18_frontend_driver_t*d=u;(void)c;wf_status_t st;
 if(e->kind==WF_R18_CONTROL_CALL)return wf_r18_frontend_driver_call(d,e->owner_rom)!=WF_OK;
 if(e->kind==WF_R18_CONTROL_WRITE16)st=store_bus(d,e->address,e->value,2);
 else if(e->kind==WF_R18_CONTROL_READ16)st=read_bus(d,e->address,&e->value,2);
 else st=WF_BLOCKED;
 if(st!=WF_OK)fatal(d,e->owner_rom,e->address);
 return st!=WF_OK;
}
static int roster_rom(void*u,uint32_t at,void*p,size_t n){return read_rom(u,at,p,n)!=WF_OK;}
static int roster_effect(void*u,uint32_t pc,uint32_t a[7],uint8_t*ram,wf_r17_m68k_shadow_t*r){
 wf_r18_frontend_driver_t*d=u;(void)ram;(void)r;memcpy(d->a,a,sizeof d->a);
 wf_status_t st=wf_r18_frontend_driver_call(d,pc);memcpy(a,d->a,sizeof d->a);return st!=WF_OK;
}
static int roster_palette(void*u,uint32_t pc,uint32_t at,uint16_t value){
 wf_r18_frontend_driver_t*d=u;wf_status_t st=store_bus(d,at,value,2);if(st!=WF_OK)fatal(d,pc,at);return st!=WF_OK;
}
static void mode_effect(void*u,const wf_r18_mode_effect_t*e,uint8_t*ram,wf_r17_m68k_shadow_t*r){
 wf_r18_frontend_driver_t*d=u;(void)ram;(void)r;
 if(d->phase==WF_R18_DRIVER_FATAL)return;
 if(e->words){for(unsigned i=0;i<e->count;i++)if(store_bus(d,e->destination+i*2u,e->words[i],2)!=WF_OK){fatal(d,e->owner_rom,e->destination+i*2u);return;}}
 else (void)wf_r18_frontend_driver_call(d,e->owner_rom);
}
wf_status_t wf_r18_frontend_driver_call(wf_r18_frontend_driver_t*d,uint32_t owner){
 if(!d||!d->ram||!d->runtime||!d->video||d->phase==WF_R18_DRIVER_FATAL)return WF_BLOCKED;
 uint32_t generation=d->generation,saved_owner=d->active_owner;wf_status_t st=WF_BLOCKED;
 d->active_owner=owner;d->call_depth++;
 switch(owner){
 case 0x2006:case 0xb53a:case 0x27012:case 0x27066:case 0x2946a:case 0x264aa:case 0x2988:{
  wf_r18_frontend_board_t b={.ram=d->ram,.ram_bytes=d->ram_bytes,.regs=&d->regs,.a=d->a,.read_rom=read_rom,.store_bus=store_bus,.read_bus=read_bus,.user=d};
  if(owner==0x2988&&wait_level(d,1,0x298c)!=WF_OK)break;
  st=wf_r18_frontend_board_call(&b,owner);
  if(st!=WF_OK&&generation==d->generation)fatal(d,b.blocked_dependency,b.blocked_address);
  if(st==WF_OK&&owner==0x2988)st=wait_level(d,0,0x29f6);
  break;
 }
 case 0x26e66:case 0x26a42:case 0x26dac:case 0x26e1a:{
  if(owner==0x26e66&&d->platform.bind_world(d->platform.user,rw(d->ram+0x7e))!=WF_OK)break;
  wf_r18_frontend_map_t m={d->ram,&d->regs,d->a,read_rom,store_bus,d,0};st=wf_r18_frontend_map_call(&m,owner);
  if(st==WF_OK&&owner==0x26e66)d->world_visible=1;
  if(st!=WF_OK)fatal(d,m.blocked_dependency,0);
  break;
 }
 case 0x247c:case 0x27b8:case 0x2836:{
  wf_r18_frontend_leaf_t leaf={read_rom,d,(wf_r18_sprite_record_t*)(void*)(d->video->fg0+0x2000),512,NULL};
  wf_r18_frontend_render_t render={d->ram,&d->regs,d->a,wf_r18_frontend_leaf_emit,wf_r18_frontend_leaf_clear,&leaf,0};
  st=wf_r18_frontend_render_call(&render,owner);
  if(st!=WF_OK)fatal(d,render.blocked_dependency,0);
  if(st==WF_OK&&owner==0x2836)st=d->platform.submit_records(d->platform.user,leaf.records,rw(d->ram+0x19b2));
  break;
 }
 case 0xb312:case 0xb4b8:{
  wf_r18_title_palette_context_t p={d->ram,&d->regs,read_rom,video_batch,d->runtime,d};
  st=owner==0xb312?wf_r18_title_palette_b312(&p):wf_r18_title_palette_irq_b4b8(&p);break;
 }
 case 0x2a06:
  if(wait_level(d,1,0x2a0a)!=WF_OK)break;
  d->runtime->m68k_regs=d->regs;st=wf_r18_frontend_palette_2a06(d->runtime,d->video,rw(d->ram+0x15f4),rw(d->ram+0x15fc));d->regs=d->runtime->m68k_regs;
  if(st==WF_OK)st=wait_level(d,0,0x2a88);
  break;
 case 0x26772:case 0x26844:case 0x264e2:st=fade(d,owner);break;
 case 0x21e6:{
  unsigned periods=(uint16_t)d->regs.d[0]+1u;st=WF_OK;
  while(periods--){if(wait_level(d,1,0x21ea)!=WF_OK||wait_level(d,0,0x21f4)!=WF_OK){st=WF_BLOCKED;break;}}
  break;
 }
 case 0x2052:
  st=WF_OK;if(rw(d->ram+0x7c)||!(d->ram[0x67]&32u))st=store_bus(d,0x14000c,(uint16_t)d->regs.d[0]|0x3100u,2);break;
 case 0x978:{
  uint16_t input[4];if(ports(d,input)!=WF_OK)break;
  wf_r18_title_context_t c={d->ram,d->ram_bytes,&d->regs,d->a,title_effect,d};
  int carry=wf_r18_frontend_start_0978(&c,input,NULL);
  if(carry>=0){d->control.sr=(uint16_t)((d->control.sr&~1u)|(carry?1u:0u));st=WF_OK;}break;
 }
 case 0x1f38:case 0x1f52:case 0x1f6c:case 0x1f9e:case 0x1fc0:case 0x1fde:
 case 0x2aea:case 0x2b58:case 0x51d2:case 0x527e:case 0x9a42:case 0x2503c:case 0x26122:case 0x9aca:case 0x1e92:{
  if((owner==0x1f6cu||owner==0x1fc0u||owner==0x1fdeu)&&d->platform.scene_clear){
   st=d->platform.scene_clear(d->platform.user,owner,0u);if(st!=WF_OK)break;
  }
  wf_r18_bootstrap_context_t bootstrap={.text_owner=text,.fg0_batch=video_batch,.user=d};
  wf_r18_frontend_io_t io={.ram=d->ram,.regs=&d->regs,.a=d->a,.runtime=d->runtime,.video=d->video,.bootstrap=&bootstrap,.user=d};
  st=wf_r18_frontend_io_call(&io,owner);
  if(st==WF_OK&&owner==0x1f6c)d->world_visible=0;
  if(st==WF_OK&&(owner==0x1fde||owner==0x1fc0))st=d->platform.submit_records(d->platform.user,(const wf_r18_sprite_record_t*)(const void*)(d->video->fg0+0x2000),0);
  /* $1F9E only clears the outgoing text. Releasing scanout here exposes
   * the old scene while the next plate is still being constructed. The
   * copyright plate is the sole boot scene without a following world bind;
   * its replacement text at $2503C is the actual reveal boundary. */
  if(st==WF_OK&&owner==0x2503cu&&d->phase==WF_R18_DRIVER_BOOT&&d->platform.scene_clear)
   st=d->platform.scene_clear(d->platform.user,owner,1u);
  break;
 }
 default:break;
 }
 d->call_depth--;d->active_owner=saved_owner;
 if(generation!=d->generation)return WF_BLOCKED;
 if(st!=WF_OK)return fatal(d,owner,0);
 return WF_OK;
}
static uint32_t scene_pc(const wf_r18_frontend_driver_t*d){
 switch(d->phase){case WF_R18_DRIVER_RESET:return d->scene.reset.pc;case WF_R18_DRIVER_BOOT:return d->scene.boot.pc;case WF_R18_DRIVER_TITLE:return d->scene.title.pc;case WF_R18_DRIVER_MODE_PLATE:return d->scene.plate.pc;case WF_R18_DRIVER_MODE:return d->scene.mode.pc;case WF_R18_DRIVER_ROSTER:return d->scene.roster.pc;default:return d->pc;}
}
wf_status_t wf_r18_frontend_driver_init(wf_r18_frontend_driver_t*d,const wf_r18_frontend_driver_config_t*c){
 if(!d||!c||!c->ram||c->ram_bytes<0x4000||!c->runtime||!c->video||!(c->sr&0x2000u)||
  !c->platform.read_rom||!c->platform.read_bus||!c->platform.store_bus||!c->platform.wait_level||!c->platform.wait_vblank||!c->platform.commit||!c->platform.bind_world||!c->platform.submit_records)return WF_BAD_STATE;
 memset(d,0,sizeof *d);d->ram=c->ram;d->ram_bytes=c->ram_bytes;d->runtime=c->runtime;d->video=c->video;d->platform=c->platform;d->regs.valid=1;
 d->control=(wf_r18_frontend_control_context_t){d->ram,d->ram_bytes,&d->regs,d->a,c->ssp,c->usp,c->sr,control_effect,d};
 d->video->read_rom=read_rom;d->video->read_ram=read_ram;d->video->read_user=d;d->video->ring_cell=NULL;
 wf_r18_frontend_reset_init(&d->scene.reset);d->pc=0x654;return WF_OK;
}
wf_status_t wf_r18_frontend_driver_on_vblank(wf_r18_frontend_driver_t*d){
 if(!d||d->phase==WF_R18_DRIVER_FATAL||d->irq_active)return WF_BLOCKED;
 uint16_t interrupted_sr=d->control.sr;uint32_t interrupted_pc=d->pc;
 d->irq_active=1;d->control.ssp-=6;d->control.sr=(uint16_t)((interrupted_sr&0x001fu)|0x2300u);
 wf_r18_frontend_irq_enter(&d->irq,interrupted_sr,interrupted_pc);int result;
 do{result=wf_r18_frontend_irq_step(&d->irq,&d->control);}while(result==WF_R18_CONTROL_MORE||result==WF_R18_CONTROL_WAIT);
 d->irq_active=0;
 if(result==WF_R18_CONTROL_HANDOFF){
  d->generation++;d->credited_start=1;d->phase=WF_R18_DRIVER_MODE_PLATE;d->fade.active=0;wf_r18_frontend_mode_plate_init(&d->scene.plate);d->pc=0x52be;
 }else if(result!=WF_R18_CONTROL_RTE)return fatal(d,d->irq.blocked_dependency?d->irq.blocked_dependency:d->irq.pc,0);
 return d->platform.commit(d->platform.user,d)==WF_OK?WF_OK:fatal(d,d->irq.pc,0);
}
wf_status_t wf_r18_frontend_driver_on_raster(wf_r18_frontend_driver_t*d){
 if(!d||d->phase==WF_R18_DRIVER_FATAL||d->coin_active)return WF_BLOCKED;
 uint16_t interrupted_sr=d->control.sr;uint32_t interrupted_pc=d->pc;
 d->coin_active=1;d->control.ssp-=6;d->control.sr=(uint16_t)((interrupted_sr&0x001fu)|0x2200u);
 wf_r18_frontend_coin_enter(&d->coin,interrupted_sr,interrupted_pc);int result;
 do{result=wf_r18_frontend_coin_step(&d->coin,&d->control);}while(result==WF_R18_CONTROL_MORE);
 d->coin_active=0;
 return result==WF_R18_CONTROL_RTE?WF_OK:fatal(d,d->coin.blocked_dependency?d->coin.blocked_dependency:d->coin.pc,0);
}
int wf_r18_frontend_driver_step(wf_r18_frontend_driver_t*d){
 if(!d||d->phase==WF_R18_DRIVER_FATAL)return WF_R18_DRIVER_BLOCKED;
 if(d->phase==WF_R18_DRIVER_AC0)return WF_R18_DRIVER_HANDOFF;
 uint32_t generation=d->generation;int result=-1;uint32_t dependency=0;d->pc=scene_pc(d);
 wf_r18_title_context_t t={d->ram,d->ram_bytes,&d->regs,d->a,title_effect,d};
 switch(d->phase){
 case WF_R18_DRIVER_RESET:
  result=wf_r18_frontend_reset_step(&d->scene.reset,&d->control);dependency=d->scene.reset.blocked_dependency;
  if(result==WF_R18_CONTROL_HANDOFF){
   if(d->platform.reset_ready&&d->platform.reset_ready(d->platform.user,d->ram,d->ram_bytes)!=WF_OK)
    return fatal(d,0x6f6u,0);
   d->phase=WF_R18_DRIVER_BOOT;wf_r18_frontend_boot_init(&d->scene.boot);result=0;
  }break;
 case WF_R18_DRIVER_BOOT:
  result=wf_r18_frontend_boot_step(&d->scene.boot,&t);dependency=d->scene.boot.blocked_dependency;
  if(generation==d->generation&&result==WF_R18_BOOT_TITLE){d->phase=WF_R18_DRIVER_TITLE;wf_r18_title_init(&d->scene.title);result=0;}break;
 case WF_R18_DRIVER_TITLE:
  result=wf_r18_title_step(&d->scene.title,&t);dependency=d->scene.title.blocked_dependency;
  if(generation==d->generation&&result==WF_R18_TITLE_ATTRACT){d->phase=WF_R18_DRIVER_AC0;result=0;}break;
 case WF_R18_DRIVER_MODE_PLATE:
  result=wf_r18_frontend_mode_plate_step(&d->scene.plate,&t);dependency=d->scene.plate.blocked_dependency;
  if(result==WF_R18_TITLE_ATTRACT){d->phase=WF_R18_DRIVER_MODE;wf_r18_mode_select_init(&d->scene.mode);result=0;}break;
 case WF_R18_DRIVER_MODE:{
  wf_r18_mode_select_context_t m={.ram_1c0000=d->ram,.ram_bytes=d->ram_bytes,.regs=&d->regs,.effect=mode_effect,.user=d};
  if(ports(d,m.input_words)!=WF_OK)return WF_R18_DRIVER_BLOCKED;
  result=wf_r18_mode_select_step(&d->scene.mode,&m);dependency=d->scene.mode.blocked_dependency;
  if(result==WF_R18_MODE_WAIT){
   uint32_t pc=d->scene.mode.pc;unsigned high=pc==0x539a||pc==0x53ca||pc==0x5406||pc==0x5436||pc==0x5778;
   if(wait_level(d,high,pc)!=WF_OK)result=-1;
   else{
    /* Resume the exact waiting branch with its observed bus value before
     * any new port sampling or frontend dispatch can consume the pulse. */
    m.input_words[3]=(uint16_t)((m.input_words[3]&~0x400u)|(high?0x400u:0u));
    result=wf_r18_mode_select_step(&d->scene.mode,&m);dependency=d->scene.mode.blocked_dependency;
   }
  }
  if(result==WF_R18_MODE_ROSTER){d->phase=WF_R18_DRIVER_ROSTER;wf_r18_roster_select_init(&d->scene.roster);result=0;}
  break;
 }
 case WF_R18_DRIVER_ROSTER:{
  wf_r18_roster_select_context_t r={.ram_1c0000=d->ram,.ram_bytes=d->ram_bytes,.regs=&d->regs,.read_rom=roster_rom,.effect=roster_effect,.user=d,.write_palette=roster_palette};memcpy(r.a,d->a,sizeof d->a);
  if(ports(d,r.input_words)!=WF_OK)return WF_R18_DRIVER_BLOCKED;
  result=wf_r18_roster_select_step(&d->scene.roster,&r);dependency=r.blocked_dependency;
  if(result==WF_R18_ROSTER_WAIT){
   uint32_t pc=d->scene.roster.pc;unsigned high=pc!=0x596e;
   wf_status_t st=pc==0x5d1e?wait_vblank(d,pc):wait_level(d,high,pc);
   if(st!=WF_OK)result=-1;
   else{
    if(pc!=0x5d1e)r.input_words[3]=(uint16_t)((r.input_words[3]&~0x400u)|(high?0x400u:0u));
    result=wf_r18_roster_select_step(&d->scene.roster,&r);dependency=r.blocked_dependency;
   }
  }
  memcpy(d->a,r.a,sizeof d->a);
  if(result==WF_R18_ROSTER_COMBAT){d->phase=WF_R18_DRIVER_AC0;result=0;}
  else if(result==WF_R18_ROSTER_LIVE)result=0;
  break;
 }
 default:break;
 }
 if(generation!=d->generation){d->pc=scene_pc(d);return WF_R18_DRIVER_MORE;}
 if(result<0||d->phase==WF_R18_DRIVER_FATAL){fatal(d,dependency?dependency:d->pc,0);return WF_R18_DRIVER_BLOCKED;}
 d->pc=d->phase==WF_R18_DRIVER_AC0?0xac0:scene_pc(d);
 return d->phase==WF_R18_DRIVER_AC0?WF_R18_DRIVER_HANDOFF:WF_R18_DRIVER_MORE;
}
wf_status_t wf_r18_frontend_driver_resume_boot(wf_r18_frontend_driver_t*d,uint32_t entry_pc){
 if(!d||entry_pc!=0x6fcu||!d->ram||d->ram_bytes<0x4000u||!d->regs.valid||
    d->phase==WF_R18_DRIVER_FATAL||d->irq_active||d->coin_active||d->call_depth||d->active_owner)
  return WF_BAD_STATE;
 wf_r18_frontend_boot_init(&d->scene.boot);d->phase=WF_R18_DRIVER_BOOT;
 d->pc=0x6fcu;d->credited_start=0u;d->generation++;return WF_OK;
}
wf_status_t wf_r18_frontend_driver_run(wf_r18_frontend_driver_t*d){
 int result;do{result=wf_r18_frontend_driver_step(d);}while(result==WF_R18_DRIVER_MORE||result==WF_R18_DRIVER_WAIT);
 return result==WF_R18_DRIVER_HANDOFF?WF_OK:WF_BLOCKED;
}
wf_status_t wf_r18_frontend_driver_attract_visual_8490(wf_r18_frontend_driver_t*d){
 if(!d||d->phase!=WF_R18_DRIVER_AC0||!d->ram||d->ram_bytes<0x4000u)return WF_BAD_STATE;
 uint32_t previous_pc=d->pc,generation=d->generation;
 uint8_t *r=d->ram,objects[12u*6u];
 /* $8490..$84D8 selects the title world without repeating the complete
  * portrait entrance. These objects come from the original $86C8 table. */
 ww(r+0x15f4u,3u);ww(r+0x15f8u,3u);d->pc=0x84a0u;
 if(wf_r18_frontend_driver_call(d,0x2a06u)!=WF_OK)return WF_BLOCKED;
 ww(r+0x7eu,3u);ww(r+0x1804u,0x280u);ww(r+0x1806u,0x400u);
 d->pc=0x84beu;if(wf_r18_frontend_driver_call(d,0x26e66u)!=WF_OK)return WF_BLOCKED;
 r[0x6fu]|=16u;d->pc=0x84ccu;
 if(wf_r18_frontend_driver_call(d,0x1f9eu)!=WF_OK)return WF_BLOCKED;
 ww(r+0x72u,0xffffu);
 if(read_rom(d,0x86c8u,objects,sizeof objects)!=WF_OK)return fatal(d,0x84f4u,0x86c8u);
 d->a[0]=0x1c1258u;wf_r17_m68k_move_w(&d->regs,0u,0u);
 for(unsigned i=0;i<12u;i++){
  uint8_t *o=r+0x1258u+i*0x2au;
  ww(o,0x8000u);wf_r17_m68k_move_l(&d->regs,1u,i*6u);
  for(unsigned j=0;j<3u;j++){
   uint16_t word=rw(objects+i*6u+j*2u);
   ww(o+(j==0u?2u:j==1u?0x14u:0x16u),word);
  }
  ww(o+4u,4u);d->a[0]+=0x2au;wf_r17_m68k_move_w(&d->regs,0u,(uint16_t)(i+1u));
 }
 uint32_t saved_d0=d->regs.d[0],saved_d1=d->regs.d[1],saved_a0=d->a[0];
 d->a[0]=0x1c1258u;wf_r17_m68k_move_w(&d->regs,1u,11u);
 for(unsigned i=0;i<12u;i++){
  wf_r17_m68k_move_w(&d->regs,0u,rw(r+0x1258u+i*0x2au+2u));
  d->pc=0x852eu;if(wf_r18_frontend_driver_call(d,0x2aeau)!=WF_OK)return WF_BLOCKED;
  wf_r17_m68k_move_w(&d->regs,0u,0x2cu);d->pc=0x8538u;
  if(wf_r18_frontend_driver_call(d,0x2aeau)!=WF_OK)return WF_BLOCKED;
  wf_r17_m68k_move_w(&d->regs,0u,0x2du);d->pc=0x8542u;
  if(wf_r18_frontend_driver_call(d,0x2aeau)!=WF_OK)return WF_BLOCKED;
  d->pc=0x8548u;if(wf_r18_frontend_driver_call(d,0x27b8u)!=WF_OK)return WF_BLOCKED;
  d->a[0]+=0x2au;wf_r17_m68k_move_w(&d->regs,1u,(uint16_t)(11u-i-1u));
 }
 d->regs.d[0]=saved_d0;d->regs.d[1]=saved_d1;d->a[0]=saved_a0;
 ww(r+0x1cd4u,(uint16_t)(rw(r+0x1cd4u)+2u));
 ww(r+0x1cd6u,(uint16_t)(rw(r+0x1cd6u)+1u));d->pc=0x856cu;
 if(wf_r18_frontend_driver_call(d,0x2836u)!=WF_OK)return WF_BLOCKED;
 ww(r+0x6eu,(uint16_t)(rw(r+0x6eu)&0x10u));r[0x6fu]|=1u;
 for(unsigned waits=0;!(r[0x6fu]&2u);waits++){
  if(waits>=8u)return fatal(d,0x8582u,0x1c006fu);
  if(wait_vblank(d,0x8582u)!=WF_OK)return WF_BLOCKED;
  if(d->generation!=generation)return WF_BLOCKED;
 }
 if(store_bus(d,0x100000u,rw(r+0x1cd4u),2u)!=WF_OK||
    store_bus(d,0x100004u,rw(r+0x1cd6u),2u)!=WF_OK)
  return fatal(d,0x858cu,0x100000u);
 ww(r+0x1cd8u,0u);wf_r17_m68k_move_w(&d->regs,0u,1u);d->pc=0x85aau;
 if(wf_r18_frontend_driver_call(d,0x2503cu)!=WF_OK)return WF_BLOCKED;
 /* $85B0 A44E and $85B6..$86C6 are the same title owners already
  * implemented at $8378..$8482, except this scene stops after 256 ticks. */
 wf_r18_title_t title={.pc=0x8378u,.blocked_dependency=0u};
 wf_r18_title_context_t context={r,d->ram_bytes,&d->regs,d->a,title_effect,d};
 for(unsigned steps=0;steps<8192u&&rw(r+0x1cd8u)<256u;steps++){
  d->pc=title.pc;int result=wf_r18_title_step(&title,&context);
  if(result<0)return fatal(d,title.blocked_dependency?title.blocked_dependency:title.pc,0);
  if(d->generation!=generation)return WF_BLOCKED;
 }
 if(rw(r+0x1cd8u)!=256u)return fatal(d,0x86c2u,0x1c1cd8u);
 d->pc=previous_pc;return WF_OK;
}
