#include "wf_r18_bootstrap.h"
#include "wf_r17_referee.h"
#include "wf_r17_participant_helpers.h"
/* Both bootstrap routes use this staging machine only for a completed
 * participant transaction. Release it before direct_boot calls finalize;
 * callback re-entry is rejected before it can reset or overwrite state. */
static wf_participant_machine_t m;
static uint8_t participant_busy;
static void dw(wf_runtime_t*r,unsigned n,uint16_t v){wf_r17_m68k_move_w(&r->m68k_regs,n,v);}
static uint16_t d(wf_runtime_t*r,unsigned n){return (uint16_t)r->m68k_regs.d[n];}
static void bit(wf_actor_raw_t*a,unsigned p,unsigned b,int set){wf_set_u8(a,p,(uint8_t)(set?(wf_u8(a,p)|(1u<<b)):(wf_u8(a,p)&~(1u<<b))));}
static int active(wf_actor_raw_t*a){return !!(wf_u8(a,0)&128);}
/* ROM $106F4..$10707; four probability distributions selected by $106E0. */
static const uint8_t weights[4][8]={{13,13,13,13,12,12,12,12},{17,17,17,17,16,16,0,0},{25,25,25,25,0,0,0,0},{50,50,0,0,0,0,0,0}};
static uint16_t draw(wf_runtime_t*r,unsigned table){
 uint32_t save1=r->m68k_regs.d[1],save2=r->m68k_regs.d[2];
 wf_r17_m68k_moveq(&r->m68k_regs,1,3);
 for(;;){wf_r17_rng21b4_live(&r->g.rng_state,r->g.frame_counter,&r->m68k_regs);r->m68k_regs.d[0]&=255u;dw(r,0,d(r,0)>>1);if(d(r,0)<100)break;dw(r,1,d(r,1)-1);if(d(r,1)==65535){dw(r,0,50);break;}}
 if(!d(r,0))dw(r,0,1);
 dw(r,1,0);dw(r,2,0);
 while(1){wf_r17_m68k_move_b(&r->m68k_regs,2,(uint8_t)(r->m68k_regs.d[2]+weights[table][d(r,1)]));if(d(r,2)>=d(r,0))break;dw(r,1,d(r,1)+1);}
 dw(r,0,d(r,1));r->m68k_regs.d[1]=save1;r->m68k_regs.d[2]=save2;return d(r,0);
}
static void health(wf_runtime_t*r,unsigned slot){
 static const uint16_t human[12]={110,106,98,102,500,500,106,104,100,96,104,96};
 static const uint16_t cpu[10]={115,145,150,155,185,160,165,165,170,200};
 static const int16_t difficulty[4]={0,-15,15,30};
 wf_actor_raw_t*a=&r->actors[slot];uint16_t value;
 if(!(wf_u8(a,0x56)&128)){int was=wf_u8(a,0x104)&128;bit(a,0x104,7,0);if(!was&&(r->g.progression_0162&255))return;value=human[wf_be16(a,0x56)&15];}
 else{value=cpu[r->g.progression_0162];if(slot!=4)value-=15;dw(r,0,value);value=(uint16_t)(value+difficulty[(r->g.config_0066>>8)&3]);}
 wf_set_be16(a,0x66,value);wf_set_be16(a,0x72,value);wf_set_be16(a,0x70,0);
}
wf_status_t wf_r18_tag_setup_1086c(wf_runtime_t*r,wf_r18_bootstrap_context_t*c){
 static const uint16_t xy[4][2]={{0x240,0x150},{0x220,0x160},{0x2c0,0x150},{0x2e0,0x160}};
 uint32_t saved[4];unsigned roster_n=0;wf_status_t st;
 if(!r||!c)return WF_BAD_STATE;
 c->blocked_pc=0;
 if(!r->m68k_regs.valid||!r->g.rng_state_valid){c->blocked_pc=0x21b4;return WF_BLOCKED;}
 if(r->g.mode_0161&1||r->g.progression_0162>=10)return WF_BAD_STATE;
 if(!c->palette_2aea){c->blocked_pc=0x2aea;return WF_BLOCKED;}
 for(unsigned i=0;i<4;i++)saved[i]=r->m68k_regs.d[i];
 if(!(active(&r->actors[0])&&active(&r->actors[2]))){
  if(!c->replay_008c){
   dw(r,0,9);for(unsigned i=0;i<10;i++){if(c->history_0598[i]==0x7c){c->history_0598[i]=c->history_0598[i+1]=65535;break;}dw(r,0,d(r,0)-1);}
   dw(r,0,0);
   while(d(r,0)<12){dw(r,1,9);for(;;){dw(r,2,d(r,1));dw(r,2,d(r,2)<<1);if(c->history_0598[d(r,1)]==d(r,0)||d(r,0)==4||d(r,0)==5)break;dw(r,1,d(r,1)-1);if(d(r,1)==65535){c->roster_0380[roster_n++]=d(r,0);break;}}dw(r,0,d(r,0)+1);}
   dw(r,0,r->g.progression_0162);if((d(r,0)&255)>=5)dw(r,0,d(r,0)-5);unsigned table=d(r,0);if(table==4)table=0;dw(r,0,d(r,0)<<2);
   draw(r,table);dw(r,1,d(r,0));draw(r,table);
   if(d(r,0)==d(r,1)){dw(r,2,d(r,1)?65535:1);dw(r,1,d(r,1)+d(r,2));}
   dw(r,0,d(r,0)<<1);dw(r,1,d(r,1)<<1);
   if(d(r,0)/2>=12||d(r,1)/2>=12)return WF_BAD_STATE;
   dw(r,0,c->roster_0380[d(r,0)/2]);dw(r,1,c->roster_0380[d(r,1)/2]);
   if(d(r,0)==10||(d(r,0)&255)==11||(d(r,1)&255)==10||(d(r,1)&255)==11){dw(r,0,10);dw(r,1,11);}
   if((r->g.progression_0162&255)==4||(r->g.progression_0162&255)==9){dw(r,0,4);dw(r,1,5);}
   wf_set_be16(&r->actors[4],0x56,d(r,0));wf_set_be16(&r->actors[5],0x56,d(r,1));wf_set_be16(&r->actors[4],2,d(r,0));wf_set_be16(&r->actors[5],2,d(r,1));
  }
  wf_set_be16(&r->actors[4],0,0x8000);wf_set_be16(&r->actors[5],0,0x8000);dw(r,2,0);
  while(1){dw(r,3,c->history_0598[d(r,2)/2]);if(d(r,3)==65535){c->history_0598[d(r,2)/2]=d(r,0);c->history_0598[d(r,2)/2+1]=d(r,1);break;}dw(r,2,d(r,2)+2);if(d(r,2)>=20)break;}
  dw(r,7,1);
  for(unsigned i=4;i<6;i++){
   wf_actor_raw_t*a=&r->actors[i];wf_set_be16(a,0x32,0);wf_set_u8(a,0x56,i==4?0x80:0xc0);r->m68k_regs.d[1]=wf_actor_rom_base(i==4?5:4);wf_set_be32(a,0x86,r->m68k_regs.d[1]);dw(r,0,wf_be16(a,2));
   st=c->palette_2aea(r,d(r,0),c->user);if(st!=WF_OK){c->blocked_pc=0x2aea;return st;}
   /* A0 remains history base $1C0598 here; +$57 reads $1C05EF. */
   if(wf_u8(&r->actors[0],0x3f)>=6)bit(&r->actors[0],0x3e,5,1);
   dw(r,7,d(r,7)-1);
  }
 }
 dw(r,7,3);
 for(unsigned i=0;i<4;i++){
  wf_actor_raw_t*a=&r->actors[i];if(active(a)){if(!(wf_u8(a,0xa1)&1))wf_set_u8(a,0x56,0x40);wf_set_be16(a,2,wf_be16(a,0x56)&255);wf_set_be16(a,0x32,0);if(!(wf_u8(a,0x56)&64))bit(a,0x33,1,1);dw(r,0,wf_be16(a,2));st=c->palette_2aea(r,d(r,0),c->user);if(st!=WF_OK){c->blocked_pc=0x2aea;return st;}if(i>=2)bit(a,0x33,7,1);}dw(r,7,d(r,7)-1);
 }
 dw(r,0,0);for(unsigned i=0;i<4;i++){wf_actor_raw_t*a=&r->actors[i];if(active(a)){health(r,i);dw(r,1,d(r,0));dw(r,1,d(r,1)<<2);wf_set_be16(a,6,xy[i][0]);wf_set_be16(a,10,xy[i][1]);wf_set_be16(a,14,0x140);if(d(r,0)>=2)bit(a,0x33,7,1);}dw(r,0,d(r,0)+1);}
 if(active(&r->actors[4])){dw(r,1,active(&r->actors[0])?8:0);unsigned k=d(r,1)/4;for(unsigned i=4;i<6;i++){wf_set_be16(&r->actors[i],6,xy[k+i-4][0]);wf_set_be16(&r->actors[i],10,xy[k+i-4][1]);wf_set_be16(&r->actors[i],14,0x140);if(k)bit(&r->actors[i],0x33,7,1);}health(r,4);health(r,5);}
 dw(r,0,2);for(unsigned i=1;i<6;i+=2){if(active(&r->actors[i]))bit(&r->actors[i],0x34,1,1);dw(r,0,d(r,0)-1);}
 if(active(&r->actors[0])){bit(&r->actors[4],0x33,7,1);bit(&r->actors[5],0x33,7,1);}
 dw(r,0,2);for(unsigned i=0;i<6;i+=2){wf_actor_raw_t*a=&r->actors[i],*p=&r->actors[i+1];if(active(a)){bit(a,0x33,0,1);if(!(wf_u8(a,0x56)&128)){bit(a,0x33,1,1);bit(a,0x56,6,0);}bit(p,0x56,6,1);if(!(wf_u8(p,0x56)&128)){if(!(wf_u8(a,0xa1)&1))bit(p,0x33,1,0);else if(wf_u8(p,0xa1)&1){bit(p,0x56,6,0);bit(p,0x33,1,1);}}}dw(r,0,d(r,0)-1);}
 wf_r17_referee_init_10718(r);
 /* $1073C..$10772 also allocate these resources, in this order. The R17
  * actor-only initializer has no video callback; retain those effects here
  * before $D1FC resolves resource 12 through the live palette map. */
 {static const uint16_t ids[]={12,7,14,29,1};
  for(unsigned i=0;i<sizeof ids/sizeof ids[0];i++){
   dw(r,0,ids[i]);st=c->palette_2aea(r,ids[i],c->user);
   if(st!=WF_OK){c->blocked_pc=0x2aea;return st;}
  }
 }
 for(unsigned i=0;i<4;i++)r->m68k_regs.d[i]=saved[i];
 c->replay_008c=0;return WF_OK;
}

#include "wf_r17_participant_helpers.h"
#include <string.h>
#include "wf_r18_bootstrap_palettes.inc"
wf_status_t wf_r18_credit_1e92(wf_runtime_t*r,wf_r18_bootstrap_context_t*c){
 if(!r||!c||!c->text_owner||!c->fg0_batch)return WF_BAD_STATE;
 uint32_t saved=r->m68k_regs.d[0];wf_status_t st;
 dw(r,0,r->g.global_0072);
 if((int16_t)d(r,0)>=0&&d(r,0)==((r->g.credit_remainder_004e<<8)|r->g.credit_count_004f)){r->m68k_regs.d[0]=saved;return WF_OK;}
 r->g.global_0072=(uint16_t)((r->g.credit_remainder_004e<<8)|r->g.credit_count_004f);
 dw(r,0,r->g.credit_count_004f);
 /* $2790, binary WORD -> packed BCD. D2/D3 saved, D4 low becomes FFFF. */
 {uint16_t value=d(r,0);uint32_t bcd=0;unsigned shift=0;do{bcd|=(uint32_t)(value%10)<<shift;value/=10;shift+=4;}while(value);r->m68k_regs.d[1]=bcd;dw(r,4,0xffff);}
 r->g.global_0063=(uint8_t)r->m68k_regs.d[1];dw(r,0,0);
 st=c->text_owner(r,0x2503c,c->user);if(st!=WF_OK){c->blocked_pc=0x2503c;return st;}
 st=c->text_owner(r,0x26122,c->user);if(st!=WF_OK){c->blocked_pc=0x26122;return st;}
 if(r->g.credit_cycle_units_004b!=1){
  if(r->g.credit_remainder_004e){dw(r,0,2);st=c->text_owner(r,0x26122,c->user);if(st!=WF_OK){c->blocked_pc=0x26122;return st;}dw(r,0,3);st=c->text_owner(r,0x26122,c->user);if(st!=WF_OK){c->blocked_pc=0x26122;return st;}dw(r,0,0x25);st=c->text_owner(r,0x2503c,c->user);if(st!=WF_OK){c->blocked_pc=0x2503c;return st;}}
  else{wf_r17_fg0_write_t writes[3]={{0xc1e94,0,4},{0xc1e98,0,4},{0xc1e9c,0,4}};st=c->fg0_batch(r,writes,3,c->user);if(st!=WF_OK){c->blocked_pc=0x1f06;return st;}}
 }
 r->m68k_regs.d[0]=saved;return WF_OK;
}
static wf_status_t hud(wf_runtime_t*r,wf_r18_bootstrap_context_t*c){
 wf_r17_fg0_write_t writes[64];size_t n=0;
 if(!r->g.credited_route||(r->g.mode_0161&1))return WF_OK;
 for(unsigned i=0;i<4;i++)if(active(&r->actors[i])){unsigned ch=wf_be16(&r->actors[i],0x56)&15,table=ch+((wf_u8(&r->actors[i],0x33)&1)?0:12);if(ch>=12)return WF_BAD_STATE;for(unsigned j=16;j-->0;)writes[n++]=(wf_r17_fg0_write_t){0x180100u+ch*128u+j*2u,boot_palettes[table][j],2};}
 return c->video_batch(r,writes,n,c->user);
}
wf_status_t wf_r18_live_finalize_8ae6(wf_runtime_t*r,wf_r18_bootstrap_context_t*c){
 static uint8_t busy;
 wf_status_t st;
 if(!r||!c)return WF_BAD_STATE;
 if(busy||participant_busy)return WF_BLOCKED;
 c->blocked_pc=0;
 if(!c->text_owner){c->blocked_pc=0x2503c;return WF_BLOCKED;}
 if(!c->video_batch){c->blocked_pc=0xc1ea;return WF_BLOCKED;}
 if(!c->fg0_batch){c->blocked_pc=0x7506;return WF_BLOCKED;}
 r->g.global_0072=0xffff;
 st=wf_r18_credit_1e92(r,c);if(st!=WF_OK){c->blocked_pc=0x1e92;return st;}
 dw(r,0,3);
 for(unsigned i=0;i<4;i++){
  if(active(&r->actors[i])&&!(wf_u8(&r->actors[i],0x56)&128)){
   uint32_t saved=r->m68k_regs.d[0];busy=1;participant_busy=1;
   wf_participant_init(&m,r,0,0);m.a[0]=wf_actor_rom_base(i);
   wf_participant_782c(&m);wf_participant_7506(&m);
   if(m.status!=WF_OK){c->blocked_pc=m.failed_address;busy=0;participant_busy=0;return m.status;}
   st=c->fg0_batch(r,m.writes,m.write_count,c->user);if(st!=WF_OK){c->blocked_pc=0x7506;busy=0;participant_busy=0;return st;}
   memcpy(r->m68k_regs.d,m.d,sizeof m.d);r->m68k_regs.d[0]=saved;busy=0;participant_busy=0;
  }
  dw(r,0,d(r,0)-1);
 }
 st=hud(r,c);if(st!=WF_OK){c->blocked_pc=0xc1ea;return st;}
 r->g.global_169e=0x8000;return WF_OK;
}

#include "wf_r17_fixed_four_10120.h"
#include "wf_r17_integration.h"
#include "wf_r18_bootstrap_scene.inc"
static wf_status_t fill(wf_runtime_t*r,wf_r18_bootstrap_context_t*c,uint32_t base,unsigned bytes,uint32_t value){
 wf_r17_fg0_write_t w[32];
 for(unsigned off=0;off<bytes;off+=128){unsigned n=(bytes-off)/4;if(n>32)n=32;for(unsigned i=0;i<n;i++)w[i]=(wf_r17_fg0_write_t){base+off+i*4,value,4};wf_status_t s=c->video_batch(r,w,n,c->user);if(s!=WF_OK)return s;}
 return WF_OK;
}
static wf_status_t scene_palette(wf_runtime_t*r,wf_r18_bootstrap_context_t*c,unsigned mode){
 wf_r17_fg0_write_t w[32];wf_status_t st;
 for(unsigned layer=0;layer<2;layer++)for(unsigned bank=0;bank<16;bank++){
  for(unsigned j=0;j<16;j++)w[j]=(wf_r17_fg0_write_t){(layer?0x186000u:0x188000u)+bank*128+j*2,boot_scene_layers[layer][bank*16+j],2};
  st=c->video_batch(r,w,16,c->user);if(st!=WF_OK)return st;
 }
 for(unsigned bank=0;bank<16;bank++){
  for(unsigned j=0;j<8;j++)w[j]=(wf_r17_fg0_write_t){0x180000u+bank*128+j*4,boot_scene_fg[mode][bank][j],4};
  st=c->video_batch(r,w,8,c->user);if(st!=WF_OK)return st;
 }
 dw(r,1,0x3c);dw(r,2,0xffff);return WF_OK;
}
static void map_globals(wf_runtime_t*r,wf_r18_bootstrap_context_t*c){
 r->g.global_17f6=r->g.global_1804;r->g.global_17fa=r->g.global_1806;
 r->g.global_17e6=r->g.global_17ea=r->g.global_17f6;r->g.global_17ee=r->g.global_17f2_word=r->g.global_17fa;
 r->g.global_17d6=r->g.global_17da=r->g.global_17de=r->g.global_17f6>>5;
 r->g.global_17d8=r->g.global_17dc=r->g.global_17e0=r->g.global_17fa>>5;
 r->g.global_17e2=r->g.global_17d6-3;r->g.global_17e4=r->g.global_17d8-12;
 r->g.global_16d4=1;r->g.global_17fe|=0x8000;r->g.global_16d2&=0x3fff;r->branchb_prev_185a[0]=0xff;
 r->m68k_regs.d[1]=0;dw(r,2,0xffff);dw(r,3,0x10);dw(r,0,0x7c);c->priority_140010=0x7c;
}
static wf_status_t fixed_init(wf_runtime_t*r,wf_r18_bootstrap_context_t*c){
 wf_actor_raw_t*a=&r->fixed_object0;
 wf_set_be16(a,0,0x8000);wf_set_be16(a,0x74,0);wf_set_u8(a,0x2e,0);wf_set_be16(a,2,15);wf_set_be16(a,6,0x40d);wf_set_be16(a,10,0x140);wf_set_be16(a,14,0x100);wf_set_be16(a,0x32,4);wf_set_be16(a,0x1c,0);
 a=&r->fixed_object1;wf_set_be16(a,0,0x8000);wf_set_be16(a,0x74,1);wf_set_be16(a,2,15);wf_set_be16(a,6,0x330);wf_set_be16(a,10,0x119);wf_set_be16(a,14,0x100);wf_set_be16(a,0x32,4);wf_set_be16(a,0x1c,0);
 dw(r,0,15);wf_status_t st=c->palette_2aea(r,15,c->user);if(st!=WF_OK)return st;wf_r17_fixed_four_init_1004c(r);return WF_OK;
}
wf_status_t wf_r18_direct_boot(wf_runtime_t*r,wf_r18_bootstrap_context_t*c,const wf_r18_launch_config_t*cfg){
 static uint8_t busy;wf_status_t st;wf_phase_binding_t*b;size_t n;
 if(!r||!c||!cfg||cfg->human>=12||cfg->partner>=12||cfg->human==cfg->partner||!cfg->registers.valid)return WF_BAD_STATE;
 if(busy||participant_busy)return WF_BLOCKED;
 c->blocked_pc=0;
 if(!c->video_batch||!c->fg0_batch||!c->palette_2aea||!c->palette_reset||!c->palette_release_2b58||!c->map_26e66||!c->text_owner){c->blocked_pc=0xc98;return WF_BLOCKED;}
 busy=1;b=r->bindings;n=r->binding_count;wf_runtime_init(r,b,n);r->m68k_regs=cfg->registers;r->g.rng_state=cfg->rng_state;r->g.rng_state_valid=1;r->g.config_0066=cfg->cabinet_config;r->g.credited_route=1;
 memset(c->history_0598,0xff,sizeof c->history_0598);memset(c->roster_0380,0,sizeof c->roster_0380);c->history_0598[10]=c->history_0598[11]=0;c->history_0598[0]=cfg->human;c->history_0598[1]=cfg->partner;c->replay_008c=0;
 /* Direct-selection policy supplies the fields preserved by $D1C. No prior-match state exists. */
 for(unsigned i=0;i<2;i++){wf_actor_raw_t*a=&r->actors[i];uint16_t id=i?cfg->partner:cfg->human;wf_set_be16(a,0,0x8000);wf_set_be16(a,2,id);wf_set_be16(a,0x56,id);wf_set_be16(a,0xa0,i?0:1);wf_set_be32(a,0x86,wf_actor_rom_base(1-i));wf_set_be32(a,0x8a,0x140020);}
 /* $D1C ends DBRA D0=FFFF; $C9E then loads stage0 and $D12 scene0. */
 dw(r,0,0);dw(r,1,0xffff);
 st=c->palette_reset(r,c->user);if(st!=WF_OK){c->blocked_pc=0x2988;goto done;}
 for(unsigned bank=0;bank<16;bank++)for(unsigned word=0;word<8;word++){static const uint32_t bases[4]={0x182000,0x188000,0x186000,0x180000};wf_r17_fg0_write_t w[4];for(unsigned port=0;port<4;port++)w[port]=(wf_r17_fg0_write_t){bases[port]+bank*128+word*4,0,4};st=c->video_batch(r,w,4,c->user);if(st!=WF_OK){c->blocked_pc=0x2988;goto done;}}
 st=fill(r,c,0xc0000,0x4000,0xffc5ff04);if(st!=WF_OK){c->blocked_pc=0x1fc0;goto done;}
 st=fill(r,c,0x80000,0x1000,0);if(st!=WF_OK){c->blocked_pc=0x1f6c;goto done;}st=fill(r,c,0x82000,0x1000,0);if(st!=WF_OK){c->blocked_pc=0x1f6c;goto done;}
 r->g.global_1804=0x1e0;r->g.global_1806=0x230;map_globals(r,c);st=c->map_26e66(r,c->user);if(st!=WF_OK){c->blocked_pc=0x26e66;goto done;}
 st=wf_r18_tag_setup_1086c(r,c);if(st!=WF_OK)goto done;
 st=fixed_init(r,c);if(st!=WF_OK){c->blocked_pc=0xffd2;goto done;}
 st=scene_palette(r,c,0);if(st!=WF_OK){c->blocked_pc=0x2a06;goto done;}
 st=fill(r,c,0xc0000,0x2000,0);if(st!=WF_OK){c->blocked_pc=0x1f9e;goto done;}r->g.global_0072|=0x8000;
 /* Visible $A654 is omitted by direct-launch policy; execute common $A778 teardown. */
 dw(r,0,0x1f);st=c->palette_release_2b58(r,0x1f,c->user);if(st!=WF_OK){c->blocked_pc=0x2b58;goto done;}
 for(uint32_t address=0xc0100;address<0xc1e00;address+=256){st=fill(r,c,address,160,0);if(st!=WF_OK){c->blocked_pc=0x9a42;goto done;}}dw(r,1,0xffff);
 participant_busy=1;wf_participant_init(&m,r,0,0);wf_participant_7506(&m);if(m.status!=WF_OK){c->blocked_pc=m.failed_address;st=m.status;goto done;}st=c->fg0_batch(r,m.writes,m.write_count,c->user);if(st!=WF_OK){c->blocked_pc=0x7506;goto done;}memcpy(r->m68k_regs.d,m.d,sizeof m.d);participant_busy=0;
 wf_set_be16(&r->compact_14ce[0],0,0);r->g.global_15fc=1;st=scene_palette(r,c,1);if(st!=WF_OK){c->blocked_pc=0x2a06;goto done;}
 /* Silent audio sink: $8EB6 then $2052 retain D0=312F and other registers. */
 dw(r,0,0x312f);
 st=wf_r18_live_finalize_8ae6(r,c);if(st!=WF_OK)goto done;
 st=wf_r17_runtime_enter_f8c(r,0);if(st!=WF_OK)c->blocked_pc=0xf8c;else r->frame_stage=3u;
done:busy=0;participant_busy=0;return st;
}
