#include "wf_r18_prematch.h"

static const uint32_t callbacks[24]={
 0xa9e2,0xaa96,0xaaea,0xa9e2,0xaaea,0xa814,0xaaea,0xa8c0,
 0xaaea,0xa814,0xa8c0,0xaaea,0xa8c0,0xaaea,0xa814,0xaaea,
 0xa8c0,0xaaea,0xa814,0xa9e2,0xaa96,0xa8ec,0xa952,0xa99a};
static const uint16_t pause_ticks[24]={
 128,0,0,32,0,0,0,8,0,0,16,0,8,0,0,0,8,0,0,32,0,0,0,0};
static const uint8_t voices[24]={
 0,0,0x7b,0,0xff,0,0x7c,0,0xff,0,0,0x7d,0,0xff,0,0x7c,0,0xff,0,0,0,0,0,0};
static const uint16_t voice_ticks[24]={
 8,8,160,8,0,8,20,8,0,8,8,40,8,0,8,20,8,0,8,8,8,8,8,8};
static const uint8_t identity_voice[12]={0x6d,0x6e,0x6f,0x74,0x73,0x73,0x71,0x70,0x72,0x75,0x73,0x73};
static const uint16_t identity_ticks[12]={64,128,128,128,128,128,128,128,64,256,64,64};
static unsigned speech_divisor=1u;
void wf_r18_prematch_speech_divisor_set(unsigned divisor){
 speech_divisor=divisor>=1u&&divisor<=4u?divisor:1u;
}
static uint16_t speech_ticks(uint16_t ticks){
 return (uint16_t)((ticks+speech_divisor-1u)/speech_divisor);
}
static const uint32_t primary[4]={0x1c05b0,0x1c06bc,0x1c07c8,0x1c08d4};
static const uint32_t fallback[4]={0x1c09e0,0x1c0aec,0x1c09e0,0x1c0aec};
static uint16_t r16(const uint8_t*p){return(uint16_t)((p[0]<<8)|p[1]);}
static uint32_t r32(const uint8_t*p){return((uint32_t)r16(p)<<16)|r16(p+2);}
static void w16(uint8_t*p,uint16_t v){p[0]=(uint8_t)(v>>8);p[1]=(uint8_t)v;}
static void w32(uint8_t*p,uint32_t v){w16(p,(uint16_t)(v>>16));w16(p+2,(uint16_t)v);}
static uint16_t inc(uint8_t*p){uint16_t v=(uint16_t)(r16(p)+1u);w16(p,v);return v;}
static void dw(wf_r18_prematch_t*c,unsigned i,uint16_t v){wf_r17_m68k_move_w(c->regs,i,v);}
static void db(wf_r18_prematch_t*c,unsigned i,uint8_t v){wf_r17_m68k_move_b(c->regs,i,v);}
static uint16_t d(wf_r18_prematch_t*c,unsigned i){return(uint16_t)c->regs->d[i];}
static void advance(uint8_t*p){p[0x1c]=0;p[0x1d]++;}
static wf_status_t blocked(wf_r18_prematch_t*c,uint32_t owner){c->dependency=owner;return WF_BLOCKED;}
static wf_status_t effect(wf_r18_prematch_t*c,uint32_t owner){
 c->dependency=owner;
 if(!c->effect)return WF_BLOCKED;
 wf_status_t st=c->effect(c->user,owner,c);
 if(st==WF_OK)c->dependency=0;
 return st;
}
/* Original $ABD8, retaining the A1 selected by the final relevant branch. */
static uint8_t *selected(wf_r18_prematch_t*c,uint8_t state){
 unsigned slot=state==4?0u:state==8?1u:state==13?2u:3u;
 c->a[1]=primary[slot];
 if(!(c->ram[c->a[1]-0x1c0000u]&0x80u))c->a[1]=fallback[slot];
 return c->ram+c->a[1]-0x1c0000u;
}

wf_status_t wf_r18_prematch_tag_step_a68a(wf_r18_prematch_t*c){
 if(!c||!c->ram||!c->regs||!c->regs->valid)return WF_BAD_STATE;
 c->dependency=0;
 uint8_t*p=c->ram+0x14ce;unsigned state=p[0x1d];
 if(state==24u)return WF_OK;
 if(state>24u||(uint16_t)(r16(p+0x1c)<<2)!=(uint16_t)(state*4u))return blocked(c,0xa7b4);
 c->a[0]=0x1c14ce;dw(c,0,(uint16_t)(r16(p+0x1c)<<2));c->a[1]=callbacks[state];
 uint32_t owner=c->a[1];
 if(owner==0xa8c0u){advance(p);return WF_OK;}
 unsigned latched=p[0x1c]&0x80u;p[0x1c]|=0x80u;
 wf_status_t st;
 if(owner==0xa814u){
  if(!latched){
   unsigned slot=state==5u?0u:state==9u?1u:state==14u?2u:3u;
   dw(c,0,(uint16_t)(slot*8u));c->a[1]=0xa8cc;c->a[2]=primary[slot];
   if(!(c->ram[c->a[2]-0x1c0000u]&0x80u))c->a[2]=fallback[slot];
   uint8_t*a=c->ram+c->a[2]-0x1c0000u;a[0x1d]=1;
   dw(c,0,0x3032);dw(c,1,r16(a+0x56));
   if((uint8_t)d(c,1)>=5u)dw(c,0,0x3031);
   st=effect(c,0x2052);if(st!=WF_OK)return st;
   if(a[0x57]==10u){c->a[2]+=0x10cu;c->ram[c->a[2]-0x1c0000u+0x1du]=1;}
   w16(p+0x22,0);
  }
  w16(p+4,state>=11u?0x800cu:12u);
  if(inc(p+0x22)>=64u){if(r16(c->ram+0x168))p[0x1d]=(uint8_t)(p[0x1d]+4u);advance(p);}
 }else if(owner==0xa8ecu||owner==0xa952u||owner==0xa99au){
  if(!latched){w16(p+0x22,0);w16(p+0x24,0);
   if(owner==0xa8ecu){w16(p+0x2a,8);w16(p+0x2c,0);}
  }
  if(owner==0xa8ecu){st=effect(c,0x2208);if(st!=WF_OK)return st;}
  dw(c,0,(uint16_t)(r16(p+0x24)+(owner==0xa8ecu?8u:owner==0xa952u?4u:13u)));w16(p+4,d(c,0));
  if(inc(p+0x22)>=12u){
   w16(p+0x22,0);
   if(inc(p+0x24)>=4u){if(owner==0xa8ecu)w16(p+0x24,0);else advance(p);}
  }
  if(owner==0xa8ecu&&r16(p+0xa)>=0x1a7u)advance(p);
 }else if(owner==0xa9e2u){
  if(!latched){
   dw(c,0,31);st=effect(c,0x2aea);if(st!=WF_OK)return st;
   w16(p,0x8001);w16(p+2,31);w16(p+6,0x280);w16(p+0xa,0x178);w16(p+0xe,0x140);w16(p+0x22,0);
  }
  w16(p+4,2);inc(p+0x22);c->a[1]=0xaa66;dw(c,0,pause_ticks[state]);
  if(d(c,0)<r16(p+0x22)){
   advance(p);
   if(p[0x1d]==1u){st=effect(c,0x9a42);if(st!=WF_OK)return st;if(c->ram[0x163])w16(p+0x1c,24);}
  }
 }else if(owner==0xaa96u){
  if(!latched){w16(p+0x22,0);w16(p+0x24,0);}
  dw(c,0,(uint16_t)(2u+r16(p+0x24)));w16(p+4,d(c,0));dw(c,0,r16(p+0x24)?32u:16u);
  if(inc(p+0x22)>d(c,0)){w16(p+0x22,0);if(inc(p+0x24)>=2u)advance(p);}
 }else if(owner==0xaaeau){
  if(!latched){
   w16(p+0x22,0);w16(p+0x24,0);w16(p+0x1e,0);
   c->a[1]=0xac56;dw(c,0,(uint16_t)state);db(c,0,voices[state]);
   if((uint8_t)d(c,0)!=0u&&(uint8_t)d(c,0)!=255u){
    dw(c,0,(uint16_t)(d(c,0)|0x3100u));st=effect(c,0x2052);if(st!=WF_OK)return st;
   }else{
    uint8_t*a=selected(c,(uint8_t)state);db(c,0,a[0x57]);c->a[1]=0xac4a;
    if(d(c,0)>=12u)return blocked(c,0xac4a);
    db(c,0,identity_voice[d(c,0)]);dw(c,0,(uint16_t)(d(c,0)|0x3100u));
    st=effect(c,0x2052);if(st!=WF_OK)return st;
    w16(c->ram+0x168,(uint8_t)d(c,0)==0x73u?0x8000u:0u);
   }
  }
  dw(c,0,p[0x25]);p[5]=(uint8_t)d(c,0);
  uint16_t timer=(uint16_t)(r16(p+0x22)-1u);w16(p+0x22,timer);
  if(timer&0x8000u){
   w16(p+0x22,6);uint32_t seed=r32(c->ram+0x5c);
   wf_r17_rng21b4_live(&seed,r32(c->ram+0x80),c->regs);w32(c->ram+0x5c,seed);
   dw(c,0,(uint16_t)(d(c,0)&15u));w16(p+0x22,(uint16_t)(6u+d(c,0)));p[0x25]^=1u;
  }
  inc(p+0x1e);c->a[1]=0xac6e;dw(c,0,speech_ticks(voice_ticks[state]));
  if(!d(c,0)){
   uint8_t*a=selected(c,(uint8_t)state);c->a[2]=0xac9e;
   dw(c,1,(uint16_t)((r16(a+0x56)&255u)<<1));
   if(d(c,1)>=24u)return blocked(c,0xac9e);
   dw(c,0,speech_ticks(identity_ticks[d(c,1)/2u]));
  }
  if(d(c,0)<r16(p+0x1e))advance(p);
 }
 return WF_OK;
}

/* $AD6C pointers, $AD9C lengths and shared $ADA8..$AE1F data. Some
 * identities deliberately use a shorter window into the same ROM table. */
static const uint16_t visual_at[12]={0xada8,0xadd4,0xade4,0xadf4,0xada8,0xada8,0xadf4,0xade4,0xadfc,0xae08,0xadfc,0xadf4};
static const uint8_t visual_count[12]={11,4,4,2,2,2,2,4,2,6,3,2};
static const uint16_t visual_data[60]={
 91,90,89,91,90,89,91,90,89,92,93,6,6,6,6,6,6,6,6,6,6,32,
 89,90,89,90,16,16,16,16,89,90,91,90,8,8,8,16,89,90,16,32,
 89,90,91,8,16,16,89,90,89,90,89,90,8,8,8,8,8,16};
wf_status_t wf_r18_prematch_actor_step_acb6(wf_r18_prematch_t*c){
 if(!c||!c->ram||!c->regs||!c->regs->valid)return WF_BAD_STATE;
 c->dependency=0;
 if(c->a[0]<0x1c05b0u||c->a[0]>0x1c0e10u||(c->a[0]-0x1c05b0u)%0x10cu)return blocked(c,0xacb6);
 uint8_t*p=c->ram+c->a[0]-0x1c0000u;
 if(!p[0x1d]){
  if(c->ram[0x161]&1u){
   if(inc(p+0x22)>=64u){w16(c->ram+0x14ea,24);dw(c,0,0x312f);return effect(c,0x2052);}
  }
  w16(p+4,(p[0x33]&0x80u)?0u:0x8000u);
  if(!(c->ram[0x161]&1u)){w16(p+0x22,0);w16(p+0x24,0);}
  return WF_OK;
 }
 c->a[2]=0xad6c;wf_r17_m68k_clr_l(c->regs,0);dw(c,0,(uint16_t)(r16(p+0x56)&255u));dw(c,1,d(c,0));
 if(d(c,0)>=12u)return blocked(c,0xad6c);
 unsigned identity=d(c,0);dw(c,0,(uint16_t)(d(c,0)<<2));c->a[1]=visual_at[identity];c->a[2]=0xad9c;db(c,0,visual_count[identity]);
 dw(c,1,(uint16_t)(r16(p+0x24)<<1));
 unsigned index=r16(p+0x24),count=visual_count[identity],base=(visual_at[identity]-0xada8u)/2u;
 if(index>=count)return blocked(c,visual_at[identity]);
 w16(p+4,visual_data[base+index]);inc(p+0x22);dw(c,0,(uint16_t)(d(c,0)<<1));c->a[1]+=c->regs->d[0];dw(c,1,visual_data[base+count+index]);
 if(d(c,1)<r16(p+0x22)){
  w16(p+0x22,0);inc(p+0x24);dw(c,0,(uint16_t)((d(c,0)>>1)-1u));
  if(d(c,0)<r16(p+0x24))w16(p+0x1c,0);
 }
 return WF_OK;
}

void wf_r18_prematch_scene_init(wf_r18_prematch_scene_t*s){
 if(s){s->pc=0xa654;s->blocked_dependency=0;s->saved_d0=0;}
}
static int scene_call(wf_r18_prematch_scene_t*s,wf_r18_prematch_t*c,uint32_t owner){
 if(effect(c,owner)!=WF_OK){s->blocked_dependency=c->dependency;return WF_R18_PREMATCH_BLOCKED;}
 return WF_R18_PREMATCH_MORE;
}
static int scene_io(wf_r18_prematch_scene_t*s,wf_r18_prematch_t*c,
 wf_r18_prematch_io_fn io,void*user,unsigned kind,uint32_t address,uint16_t*value){
 if(io(user,kind,s->pc,address,value,c)!=WF_OK){
  s->blocked_dependency=s->pc;c->dependency=s->pc;return WF_R18_PREMATCH_BLOCKED;
 }
 return WF_R18_PREMATCH_MORE;
}
int wf_r18_prematch_scene_step(wf_r18_prematch_scene_t*s,wf_r18_prematch_t*c,
 wf_r18_prematch_io_fn io,void*user){
 if(!s||!c||!c->ram||!c->regs||!c->regs->valid||!io)return WF_R18_PREMATCH_BAD_CONTEXT;
 uint8_t*ram=c->ram;uint8_t*p=ram+0x14ce;uint16_t value;
 c->dependency=0;s->blocked_dependency=0;
 switch(s->pc){
 case 0xa654:
  if(!ram[0x163])w16(ram+0x11fa,(uint16_t)(r16(ram+0x11fa)+0x80u));
  s->pc=0xa666;break;
 case 0xa666:
  if(!(ram[0x161]&1u)&&scene_call(s,c,0x98ba))return WF_R18_PREMATCH_BLOCKED;
  s->pc=0xa676;break;
 case 0xa676:
  dw(c,0,0x3132);if(scene_call(s,c,0x2052))return WF_R18_PREMATCH_BLOCKED;
  s->pc=0xa680;break;
 case 0xa680:s->pc=(ram[0x161]&1u)?0xa6ccu:0xa68au;break;
 case 0xa68a:
  if(wf_r18_prematch_tag_step_a68a(c)!=WF_OK){s->blocked_dependency=c->dependency;return WF_R18_PREMATCH_BLOCKED;}
  s->pc=0xa6a2;break;
 case 0xa6a2:
  if(p[0x1d]>=11u)w16(p+4,(uint16_t)(r16(p+4)|0x8000u));
  if(ram[0x163])w16(p+4,0xffff);
  s->pc=0xa6c0;break;
 case 0xa6c0:
  if(scene_call(s,c,0x247c))return WF_R18_PREMATCH_BLOCKED;
  s->pc=0xa6c6;break;
 case 0xa6c6:
  if(scene_call(s,c,0x27b8))return WF_R18_PREMATCH_BLOCKED;
  s->pc=0xa6cc;break;
 case 0xa6cc:dw(c,0,8);c->a[0]=0x1c05b0;s->pc=0xa6d6;break;
 case 0xa6d6:
  s->saved_d0=c->regs->d[0];
  if(c->a[0]<0x1c05b0u||c->a[0]>0x1c0e10u||(c->a[0]-0x1c05b0u)%0x10cu){
   s->blocked_dependency=0xa6d6;c->dependency=0xa6d6;return WF_R18_PREMATCH_BLOCKED;
  }
  if(!(ram[c->a[0]-0x1c0000u]&0x80u)){s->pc=0xa6f4;break;}
  if(wf_r18_prematch_actor_step_acb6(c)!=WF_OK){s->blocked_dependency=c->dependency;return WF_R18_PREMATCH_BLOCKED;}
  s->pc=0xa6e8;break;
 case 0xa6e8:
  if(scene_call(s,c,0x247c))return WF_R18_PREMATCH_BLOCKED;
  s->pc=0xa6ee;break;
 case 0xa6ee:
  if(scene_call(s,c,0x27b8))return WF_R18_PREMATCH_BLOCKED;
  s->pc=0xa6f4;break;
 case 0xa6f4:
  c->a[0]+=0x10cu;c->regs->d[0]=s->saved_d0;
  dw(c,0,(uint16_t)(d(c,0)-1u));s->pc=d(c,0)==0xffffu?0xa702u:0xa6d6u;break;
 case 0xa702:
  c->a[0]=0x1c11f4;if(scene_call(s,c,0x247c))return WF_R18_PREMATCH_BLOCKED;
  s->pc=0xa70e;break;
 case 0xa70e:
  if(scene_call(s,c,0x27b8))return WF_R18_PREMATCH_BLOCKED;
  s->pc=0xa714;break;
 case 0xa714:
  if(scene_call(s,c,0x2836))return WF_R18_PREMATCH_BLOCKED;
  s->pc=0xa71a;break;
 case 0xa71a:w16(ram+0x6e,0);ram[0x6f]|=1u;s->pc=0xa728;break;
 case 0xa728:
  if(!(ram[0x6f]&2u)){
   value=2;if(scene_io(s,c,io,user,WF_R18_PREMATCH_SYNC,0x1c006f,&value))return WF_R18_PREMATCH_BLOCKED;
   if(!(ram[0x6f]&2u))return WF_R18_PREMATCH_WAIT;
  }
  s->pc=0xa732;break;
 case 0xa732:case 0xa73e:case 0xa74a:case 0xa756:{
  unsigned reg=(s->pc-0xa732u)/12u;value=0;
  if(scene_io(s,c,io,user,WF_R18_PREMATCH_READ16,0x140020u+reg*2u,&value))return WF_R18_PREMATCH_BLOCKED;
  dw(c,reg,(uint16_t)((uint16_t)~value&0x30u));s->pc=reg==3u?0xa762u:s->pc+12u;break;
 }
 case 0xa762:
  dw(c,0,(uint16_t)(d(c,0)|d(c,1)|d(c,2)|d(c,3)));
  s->pc=d(c,0)||p[0x1d]>=24u?0xa778u:0xa680u;break;
 case 0xa778:
  dw(c,0,31);if(scene_call(s,c,0x2b58))return WF_R18_PREMATCH_BLOCKED;
  s->pc=0xa782;break;
 case 0xa782:
  if(scene_call(s,c,0x9a42))return WF_R18_PREMATCH_BLOCKED;
  s->pc=0xa788;break;
 case 0xa788:
  if(scene_call(s,c,0x7506))return WF_R18_PREMATCH_BLOCKED;
  s->pc=0xa78e;break;
 case 0xa78e:w16(ram+0x14ce,0);w16(ram+0x15fc,1);s->pc=0xa79c;break;
 case 0xa79c:
  if(scene_call(s,c,0x2a06))return WF_R18_PREMATCH_BLOCKED;
  s->pc=0xa7a2;break;
 case 0xa7a2:
  if(scene_call(s,c,0x8eb6))return WF_R18_PREMATCH_BLOCKED;
  s->pc=0xa7a8;break;
 case 0xa7a8:
  dw(c,0,0x312f);if(scene_call(s,c,0x2052))return WF_R18_PREMATCH_BLOCKED;
  s->pc=0xa7b2;return WF_R18_PREMATCH_DONE;
 case 0xa7b2:return WF_R18_PREMATCH_DONE;
 default:s->blocked_dependency=s->pc;c->dependency=s->pc;return WF_R18_PREMATCH_BLOCKED;
 }
 return WF_R18_PREMATCH_MORE;
}
