/* Controlled bus replay of the real HW1 owners through the production session.
 * Derived from HW1 wp1980_launch_test.c. No production autoplayer is linked.
 * Inputs and initial RAM are test fixtures, not Saturn bootstrap policy. */
#include "wf_session.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static wf_session_t session;
static uint8_t rom_data[0x80000],cold[0x4000],scroll[0x20],control[0x20];
static uint16_t backdrop;
static unsigned high,clock_count,world_mode,commits,submissions,sound_commands;
static unsigned royal,target[2],player_port,coin_state,denied_start_seen;
static unsigned last_actor=~0u,last_slot=~0u,release_until;
static unsigned last_scroll;
static unsigned need_release;
static unsigned cached_clock=~0u;static uint16_t cached_ports[4];
static unsigned stride=1,divert,diversion_seen;
static unsigned reject_match_world;
static uint16_t rw(const uint8_t*p){return (uint16_t)((p[0]<<8)|p[1]);}
static uint32_t rl(const uint8_t*p){return ((uint32_t)rw(p)<<16)|rw(p+2);}
static wf_status_t read_rom(void*u,uint32_t at,void*out,size_t n){
 (void)u;if(at>sizeof rom_data||n>sizeof rom_data-at)return WF_BLOCKED;
 memcpy(out,rom_data+at,n);return WF_OK;
}
static void inputs(void){
 if(cached_clock==clock_count)return;
 cached_clock=clock_count;
 for(unsigned i=0;i<4;i++)cached_ports[i]=i==1?0xfcff:0xffff;
 uint8_t *raw=session.ram;unsigned phase=session.frontend.phase;
 if(divert&&session.phase==WF_SESSION_MATCH_READY)diversion_seen=1;
 if(phase==WF_R18_DRIVER_BOOT||phase==WF_R18_DRIVER_TITLE||
    (phase==WF_R18_DRIVER_AC0&&!rw(raw+0x7c))){
  if(clock_count<24){cached_ports[player_port]&=~0x80u;return;}
  if(!rw(raw+0x7c)&&raw[0x4f]==0)denied_start_seen=1;
  if(divert&&!diversion_seen)return;
  if(!raw[0x4f]){
   if(coin_state==0&&raw[0x10]>=4)coin_state=1;
   if(coin_state==1&&rw(raw+0x54))coin_state=2;
   if(coin_state==1)cached_ports[0]&=~0x100u;
  }else cached_ports[player_port]&=~0x80u;
 }else if(phase==WF_R18_DRIVER_MODE){
  if((raw[0x161]&1u)!=royal)cached_ports[player_port]&=~(royal?2u:1u);
  else cached_ports[player_port]&=~0x10u;
 }else if(phase==WF_R18_DRIVER_ROSTER){
  unsigned chosen=~0u,confirmed=0;
  for(unsigned i=0;i<4;i++){
   unsigned at=0x5b0+i*0x10c;
   if(!(raw[at]&0x80u)||rl(raw+at+0x8a)!=0x140020u+2u*player_port)continue;
   if(raw[at+0x33]&1u)confirmed++;
   else if(chosen==~0u)chosen=at;
  }
  if(chosen==~0u)return;
  unsigned slot=rw(raw+chosen+0x22),goal=target[confirmed?1:0];
  unsigned scrolling=raw[0x15e]&8u;
  if(chosen!=last_actor||slot!=last_slot||scrolling!=last_scroll){last_actor=chosen;last_slot=slot;last_scroll=scrolling;release_until=clock_count+2;need_release=1;}
  if(need_release){
   /* Hold neutral until the original input owner really samples release;
    * elapsed display events alone do not prove that the pad was polled. */
   if(rw(raw+chosen+0xa6)||raw[chosen+0x91])return;
   need_release=0;
  }
  if(clock_count<release_until)return;
  if(slot==goal){
   if(!(raw[chosen+0xa7]&3u))cached_ports[player_port]&=~0x10u;
  }else{
   unsigned direction=(slot%5u<goal%5u)?1u:(slot%5u>goal%5u)?2u:(slot<goal)?8u:4u;
   cached_ports[player_port]&=(uint16_t)~direction;
  }
 }else if(phase==WF_R18_DRIVER_AC0){
  if((clock_count&7u)<4)cached_ports[player_port]&=~0x10u;
 }
}
static uint16_t port_word(unsigned port){
 inputs();uint16_t word=cached_ports[port];
 if(port==3)word=(uint16_t)((word&~0x400u)|(high?0x400u:0));
 return word;
}
static wf_status_t read_bus(void*u,uint32_t at,uint32_t*out,unsigned n){
 (void)u;*out=0;
 for(unsigned i=0;i<n;i++){
  uint32_t a=at+i;uint8_t b;
  if(a>=0x140020&&a<0x140028){uint16_t w=port_word((a-0x140020)/2);b=(uint8_t)(w>>((a&1)?0:8));}
  else if(a>=0x100000&&a<0x100020)b=scroll[a-0x100000];
  else if(a>=0x140000&&a<0x140020)b=control[a-0x140000];
  else if(a>=0x184000&&a<0x184002)b=(uint8_t)(backdrop>>((a&1)?0:8));
  else return WF_BLOCKED;
  *out=(*out<<8)|b;
 }return WF_OK;
}
static wf_status_t store_bus(void*u,uint32_t at,uint32_t v,unsigned n){
 (void)u;uint8_t*p=NULL;
 if(at>=0x100000&&at+n<=0x100020)p=scroll+(at-0x100000);
 else if(at>=0x140000&&at+n<=0x140020)p=control+(at-0x140000);
 else if(at==0x184000&&n==2){backdrop=(uint16_t)v;return WF_OK;}
 if(!p)return WF_BLOCKED;
 for(unsigned i=0;i<n;i++)p[i]=(uint8_t)(v>>(8*(n-i-1)));
 if(at==0x14000c)sound_commands++;
 return WF_OK;
}
static wf_status_t tick(wf_r18_frontend_driver_t*d){
 for(unsigned j=0;j<stride;j++){
  high=1;++clock_count;
  if(clock_count>30000){fprintf(stderr,"limit pc=%x phase=%u slot=%u/%u credits=%u\n",d->pc,d->phase,rw(session.ram+0x5d2),rw(session.ram+0x6de),session.ram[0x4f]);exit(2);}
  for(unsigned i=0;i<17;i++)if((d->control.sr&0x700)<0x200)
   if(wf_r18_frontend_driver_on_raster(d)!=WF_OK)return WF_BLOCKED;
  if((d->control.sr&0x700)<0x300)
   if(wf_r18_frontend_driver_on_vblank(d)!=WF_OK)return WF_BLOCKED;
 }return WF_OK;
}
static wf_status_t wait_level(void*u,wf_r18_frontend_driver_t*d,unsigned level){
 (void)u;if(level&&!high)return tick(d);high=level;return WF_OK;
}
static wf_status_t wait_vblank(void*u,wf_r18_frontend_driver_t*d){(void)u;high=0;return tick(d);}
static wf_status_t commit(void*u,wf_r18_frontend_driver_t*d){(void)u;(void)d;commits++;return WF_OK;}
static wf_status_t bind_world(void*u,unsigned mode){
 (void)u;if(reject_match_world&&session.phase==WF_SESSION_MATCH_READY)return WF_BLOCKED;
 world_mode=mode;return WF_OK;
}
static wf_status_t submit(void*u,const wf_r18_sprite_record_t*r,size_t n){
 (void)u;assert((const uint8_t*)r==session.video.fg0+0x2000);submissions++;return n<=512?WF_OK:WF_BLOCKED;
}
static void snapshot(const char*directory,const char*name){
 char path[2048];snprintf(path,sizeof path,"%s/%s.ram",directory,name);FILE*f=fopen(path,"wb");assert(f);assert(fwrite(session.ram,1,0x4000,f)==0x4000);assert(!fclose(f));
 snprintf(path,sizeof path,"%s/%s.json",directory,name);f=fopen(path,"w");assert(f);
 fprintf(f,"{\"pc\":%u,\"ssp\":%u,\"usp\":%u,\"sr\":%u,\"generation\":%u,\"clock\":%u,\"d\":[",session.frontend.pc,session.frontend.control.ssp,session.frontend.control.usp,session.frontend.control.sr,session.frontend.generation,clock_count);
 for(unsigned i=0;i<8;i++)fprintf(f,"%s%u",i?",":"",session.frontend.regs.d[i]);fputs("],\"a\":[",f);
 for(unsigned i=0;i<7;i++)fprintf(f,"%s%u",i?",":"",session.frontend.a[i]);fputs("]}\n",f);assert(!fclose(f));
}
int main(int argc,char**argv){
 assert(argc==10);royal=(unsigned)strtoul(argv[3],NULL,0);target[0]=(unsigned)strtoul(argv[4],NULL,0);target[1]=(unsigned)strtoul(argv[5],NULL,0);player_port=(unsigned)strtoul(argv[6],NULL,0);
 uint32_t seed=(uint32_t)strtoul(argv[7],NULL,0);stride=(unsigned)strtoul(argv[8],NULL,0);divert=(unsigned)strtoul(argv[9],NULL,0);
 reject_match_world=getenv("WF_SESSION_REJECT_MATCH_WORLD")!=NULL;
 assert(royal<=1&&target[0]<10&&target[1]<10&&player_port<4&&stride>=1&&stride<=8);
 FILE*f=fopen(argv[1],"rb");assert(f);assert(fread(rom_data,1,sizeof rom_data,f)==sizeof rom_data);assert(!fclose(f));
 for(unsigned i=0;i<4;i++)cold[0x5c+i]=(uint8_t)(seed>>(24-8*i));
 wf_session_config_t config={.cold_ram=cold,.cold_ram_bytes=sizeof cold,.ssp=0x1c3bd4,.usp=0x1c3fd4,.sr=0x2700,
  .platform={.read_rom=read_rom,.read_bus=read_bus,.store_bus=store_bus,.wait_level=wait_level,.wait_vblank=wait_vblank,.commit=commit,.bind_world=bind_world,.submit_records=submit}};
 assert(wf_session_init(&session,&config)==WF_OK);
 unsigned guard=0,cancellations=0;
 for(;;){
  while(session.phase==WF_SESSION_FRONTEND){
   if(wf_session_step(&session)!=WF_OK){fprintf(stderr,"frontend failed %x/%x\n",session.error_pc,session.error_owner);return 3;}
   assert(++guard<2000000);
  }
  assert(session.phase==WF_SESSION_MATCH_READY);snapshot(argv[2],"selection");
  if(wf_session_prepare_match(&session)!=WF_OK){
   if(reject_match_world){
    assert(session.phase==WF_SESSION_FATAL&&session.error_owner&&session.runtime.frame_stage!=3);
    unsigned previous=clock_count;assert(wf_session_prepare_match(&session)==WF_BLOCKED&&clock_count==previous);
    printf("PASS expected match dependency failure pc=%x owner=%x; no handoff/replay\n",session.error_pc,session.error_owner);return 0;
   }
   fprintf(stderr,"match failed %x/%x\n",session.error_pc,session.error_owner);return 4;
  }
  if(session.phase==WF_SESSION_FRONTEND){cancellations++;continue;}break;
 }
 assert(session.phase==WF_SESSION_GAMEPLAY&&session.runtime.frame_stage==3);
 assert(!reject_match_world);
 assert(rw(session.ram+0x7c)==0x8000&&denied_start_seen);
 assert((session.runtime.g.mode_0161&1u)==royal);
 assert(session.match.machine.ram==session.ram&&session.match.machine.regs==&session.frontend.regs);
 static const unsigned ids[10]={0,7,2,8,6,9,1,3,10,11};
 /* ROM $5DA0 swaps the uncredited lower actor with credited P2. The
  * special TAG pair starts as fixed slots8/9, so P2 leaves raw11/10. */
 unsigned first=ids[target[0]],second=target[0]==8?11:ids[target[1]];
 if(!royal&&target[0]==8&&player_port==1){first=11;second=10;}
 assert(wf_be16(&session.runtime.actors[0],WF_A_CHARACTER)==first);
 if(!royal)assert(wf_be16(&session.runtime.actors[1],WF_A_CHARACTER)==second);
 unsigned active=0;for(unsigned i=0;i<WF_ACTOR_SLOTS;i++)if(wf_u8(&session.runtime.actors[i],WF_A_ACTIVE)&0x80)active++;
 assert(active==(royal?6u:4u));assert(!divert||cancellations==1);
 snapshot(argv[2],"gameplay");
 printf("PASS session mode=%u targets=%u,%u port=%u fixture_seed=%u stride=%u clock=%u active=%u world=%u commits=%u submissions=%u sound_events=%u cancellations=%u rng=%08x\n",royal,target[0],target[1],player_port,seed,stride,clock_count,active,world_mode,commits,submissions,sound_commands,cancellations,session.runtime.g.rng_state);
}
