#include "wf_scsp_ym.h"
#include <string.h>
#include "generated/wf_ym_tables.h"
#include "generated/wf_ym_envelope.h"
/* Keep SH2 hot placement; let68000 --gc-sections discard unused functions. */
#if defined(__m68k__)
#define WF_YM_HOT
#else
#define WF_YM_HOT __attribute__((section(".wf_hotcode")))
#endif
static const uint8_t order[4]={0,2,1,3}; /* YM register order -> signal order */
static uint32_t WF_YM_HOT kcode(unsigned kc,unsigned kf,unsigned dt2)
{
 uint32_t k=(kc>>4)*1024u+wf_ym_kcode_map[(dt2&3u)*1024u+(kc&15u)*64u+((kf>>2)&63u)];
 return k>8127u?8127u:k;
}
uint32_t WF_YM_HOT wf_ym_phase_increment(unsigned kc,unsigned kf,unsigned dtmul,unsigned dt2)
{
 static const uint8_t detunes[8]={16,17,19,20,22,24,27,29};
 uint32_t k=kcode(kc&127u,kf,dt2),fine=wf_ym_fnum[k&1023u],key=k>>8;
 uint32_t base=(fine<<(k>>10))>>2,dt=(dtmul>>4)&7u,low=dt&3u;
 if(low){
  uint32_t capped=key>28u?28u:key;
  unsigned sum=(capped>>2)+9u+((low==3u)|(low&2u));
  uint32_t delta=detunes[((sum&1u)<<2)|(capped&3u)]>>(9u-(sum>>1));
  base=(dt&4u)?base-delta:base+delta;
 }
 base&=0x1ffffu;
 return ((dtmul&15u)?base*(dtmul&15u):base>>1)&0xfffffu;
}
uint16_t WF_YM_HOT wf_ym_pitch_word(uint32_t inc)
{
 /* YM2151 divides the board clock by2 before its32-operator pipeline. */
 int octave=-1;uint32_t q;
 if(!inc)return 0x4000u;
 while(inc<4096u){inc<<=1;--octave;}
 while(inc>=8192u){inc=(inc+1u)>>1;++octave;}
 q=(inc*20779u+32768u)>>16;
 if(q>=2048u){q=(q+1u)>>1;++octave;}
 if(q>=2048u){q=1024u;++octave;}
 if(octave< -8)return 0x4000u;
 if(octave>7)return 0x3bffu;
 return (uint16_t)(((unsigned)octave&15u)*2048u+(q-1024u));
}
static void WF_YM_HOT output(wf_scsp_ym_t *s,unsigned slot,unsigned word,uint16_t value)
{
 ++s->writes;
 if(!s->write16||!s->write16(0x100000u+slot*32u+word*2u,value,s->user))s->fault=1;
}
static void WF_YM_HOT set(wf_scsp_ym_t *s,unsigned slot,unsigned word,uint16_t value)
{
 if(s->words[slot][word]!=value){s->words[slot][word]=value;output(s,slot,word,value);}
}
static void WF_YM_HOT execute(wf_scsp_ym_t *s,unsigned slot){output(s,slot,0,(uint16_t)(s->words[slot][0]|0x1000u));}
static void WF_YM_HOT route(wf_scsp_ym_t *,unsigned);
static void WF_YM_HOT frequency(wf_scsp_ym_t *,unsigned,unsigned);
static void WF_YM_HOT lfo(wf_scsp_ym_t *,unsigned);
void wf_scsp_ym_init(wf_scsp_ym_t *s,wf_sfx_write16_fn write16,void *user)
{
 memset(s,0,sizeof *s);s->write16=write16;s->user=user;
 for(unsigned ch=0;ch<8;ch++)s->reg[0x20+ch]=0xc0;
 for(unsigned slot=0;slot<32;slot++){
  s->reg[0x60+(slot%4u)*8u+slot/4u]=127;
  /* Match the shadow TL127 exactly; repeated writes are elided below. */
  uint16_t *w=s->words[slot];w[0]=0x21;w[1]=0x1200;w[3]=128;w[5]=0x3c00;w[6]=254;
  w[11]=slot<12?0xe0:0;
  for(unsigned i=0;i<12;i++)output(s,slot,i,w[i]);
 }
 for(unsigned ch=0;ch<8;ch++){
  route(s,ch);for(unsigned op=0;op<4;op++)frequency(s,ch,op);lfo(s,ch);
 }
 execute(s,31);
}
void wf_scsp_ym_stop(wf_scsp_ym_t *s)
{
 for(unsigned i=0;i<32;i++)set(s,i,0,(uint16_t)(s->words[i][0]&0xf7ffu));
 memset(s->key,0,sizeof s->key);execute(s,31);
}
static unsigned WF_YM_HOT selector(unsigned source,unsigned target,bool latest)
{
 int d=(int)source-(int)target;bool current=!(d<0||d>=28);
 return ((unsigned)d&31u)|((current==latest)?32u:0u);
}
static void WF_YM_HOT route(wf_scsp_ym_t *s,unsigned ch)
{
 /* Sources in signal order M1,C1,M2,C2. Algorithm1/2/3 need two inputs. */
 static const uint8_t inputs[8][4]={{0,1,2,4},{0,0,3,4},{0,0,2,5},{0,1,0,6},{0,1,0,4},{0,1,1,1},{0,1,0,0},{0,0,0,0}};
 static const uint8_t carriers[8]={8,8,8,8,10,14,14,15};
 unsigned conf=s->reg[0x20+ch],alg=conf&7u,fb=(conf>>3)&7u,rl=conf>>6;
 for(unsigned op=0;op<4;op++){
  unsigned slot=ch*4u+op,mask=inputs[alg][op];uint16_t modulation=0;
  /* Native operators are nominally 4x OPM amplitude; the wave has128
   * samples versus1024 OPM phase steps. MDL9 duplicates one source, MDL10
   * sums two. Feedback requires FB+1; FB1..3 map to disabled MDL2..4. */
  if(op==0&&fb)modulation=(uint16_t)(((fb+1u)<<12)|(selector(slot,slot,true)<<6)|selector(slot,slot,false));
  else if(mask){
   unsigned a=0,b=0,count=0;
   for(unsigned k=0;k<4;k++)if(mask&(1u<<k)){if(!count)a=k;else b=k;++count;}
   if(count==1)b=a;
   modulation=(uint16_t)(((count==1?9u:10u)<<12)|(selector(ch*4+a,slot,true)<<6)|selector(ch*4+b,slot,true));
  }
  /* MIXS is20bit. IMXL5 leaves headroom for four carriers before conversion
   * to24bit DSP input; IMXL7 could overflow a four-carrier channel. */
  uint16_t input=(uint16_t)(((carriers[alg]&(1u<<op))&&rl)?((8u+ch)<<3)|5u:0u);
  set(s,slot,7,modulation);set(s,slot,10,input);
 }
 unsigned pan=rl==1?0x1fu:(rl==2?0x0fu:0u);
 set(s,4u+ch,11,(uint16_t)((rl?0xe0u:0u)|pan));
}
static inline unsigned rate(unsigned raw,unsigned scaling,const uint8_t *map)
{
 if(!raw)return 0;
 unsigned effective=raw*2u+scaling;return map[effective>63u?63u:effective];
}
static void WF_YM_HOT envelope(wf_scsp_ym_t *s,unsigned ch,unsigned op)
{
 unsigned offset=op*8+ch,a=s->reg[0x80+offset],d1=s->reg[0xa0+offset],d2=s->reg[0xc0+offset],rr=s->reg[0xe0+offset];
 unsigned key=kcode(s->reg[0x28+ch]&127u,s->reg[0x30+ch],d2>>6)>>8;
 unsigned scale=key>>(3u-(a>>6)),level=rr>>4,slot=ch*4u+order[op];
 unsigned ar=rate(a&31u,scale,wf_ym_attack_rate),d1r=rate(d1&31u,scale,wf_ym_decay_rate);
 unsigned d2r=rate(d2&31u,scale,wf_ym_decay_rate),release=rate((rr&15u)*2u+1u,scale,wf_ym_decay_rate),krs=15;
 /* OPM effective AR62/63 starts at zero attenuation. Native key-on does
  * so when AR+key-scale>=32. Compensate every other nonzero rate so the
  * original decay/release quantization is unchanged. A raw zero stays
  * disabled; rates too small for this scale retain the ordinary mapping. */
 if((a&31u)*2u+scale>=62u){
  int octave=(s->words[slot][8]>>11)&15u;if(octave&8)octave-=16;
  unsigned native_scale=octave>0?(unsigned)octave:1u;
  if((!d1r||d1r>native_scale)&&(!d2r||d2r>native_scale)&&(!release||release>native_scale)){
   krs=(unsigned)((int)native_scale-octave);ar=31;
   if(d1r)d1r-=native_scale;
   if(d2r)d2r-=native_scale;
   if(release)release-=native_scale;
  }
 }
 uint16_t e1=(uint16_t)((d2r<<11)|(d1r<<6)|ar);
 uint16_t e2=(uint16_t)((krs<<10)|((level==15u?31u:level)<<5)|release);
 set(s,slot,4,e1);set(s,slot,5,e2);
}
static void WF_YM_HOT frequency(wf_scsp_ym_t *s,unsigned ch,unsigned op)
{
 unsigned offset=op*8u+ch;
 uint32_t inc=wf_ym_phase_increment(s->reg[0x28+ch],s->reg[0x30+ch],s->reg[0x40+offset],s->reg[0xc0+offset]>>6);
 set(s,ch*4u+order[op],8,wf_ym_pitch_word(inc));
 envelope(s,ch,op);
}
static unsigned WF_YM_HOT nearest(unsigned wanted,const uint16_t *table,unsigned count)
{
 unsigned best=0,diff=~0u;
 for(unsigned i=0;i<count;i++){unsigned d=wanted>table[i]?wanted-table[i]:table[i]-wanted;if(d<diff){diff=d;best=i;}}
 return best;
}
static void WF_YM_HOT lfo(wf_scsp_ym_t *s,unsigned ch)
{
 static const uint16_t pm[8]={0,700,1350,2700,5500,11200,23000,49400};
 static const uint16_t am[8]={0,40,80,150,300,600,1200,2400};
 static const uint16_t opm_pm[8]={0,3,7,15,31,63,254,508};
 unsigned sens=s->reg[0x38+ch],pms=(sens>>4)&7u,wave=s->reg[0x1b]&3u;
 unsigned wanted=(uint32_t)opm_pm[pms]*s->pmd*10000u/(127u*64u),depth=nearest(wanted,pm,8);
 unsigned ams=sens&3u,amplitude=ams?(unsigned)s->amd*(1u<<(ams-1u))*2400u/127u:0;
 unsigned ampdepth=nearest(amplitude,am,8),freq=wf_ym_lfo_rate[s->reg[0x18]];
 for(unsigned op=0;op<4;op++){
  unsigned enabled=s->reg[0xa0+op*8u+ch]&128u;
  uint16_t word=(uint16_t)((freq<<10)|(wave<<8)|(depth<<5)|(wave<<3)|(enabled?ampdepth:0));
  set(s,ch*4u+order[op],9,word);
 }
}
bool WF_YM_HOT wf_scsp_ym_write(wf_scsp_ym_t *s,uint8_t reg,uint8_t value)
{
 if(!s||s->fault)return false;
 /* Reset words are synchronized above, so repeated level/pitch/config
  * writes cannot change hardware. KEYON repeats also do not retrigger YM. */
 if(reg==8&&s->key[value&7u]==((value>>3)&15u)){s->reg[reg]=value;return true;}
 if((reg>=0x20||reg==0x18||reg==0x1b)&&s->reg[reg]==value)return true;
 if(reg==0x19&&((value&128u)?s->pmd== (value&127u):s->amd==value)){s->reg[reg]=value;return true;}
 s->reg[reg]=value;
 if(reg==8){
  unsigned ch=value&7u,key=(value>>3)&15u;s->key[ch]=(uint8_t)key;
  for(unsigned op=0;op<4;op++){
   unsigned slot=ch*4u+op;uint16_t w=(uint16_t)(s->words[slot][0]&0xf7ffu);
   if(key&(1u<<op))w|=0x800u;
   set(s,slot,0,w);
  }
  execute(s,ch*4u+3u);
 }else if(reg>=0x20&&reg<0x28)route(s,reg&7u);
 else if(reg>=0x28&&reg<0x38){for(unsigned op=0;op<4;op++)frequency(s,reg&7u,op);}
 else if(reg>=0x38&&reg<0x40)lfo(s,reg&7u);
 else if(reg>=0x40){
  unsigned ch=reg&7u,op=(reg>>3)&3u,group=reg&0xe0u;
  if(group==0x40u||group==0xc0u)frequency(s,ch,op);
  else if(group==0x60u)set(s,ch*4u+order[op],6,(uint16_t)((value&127u)*2u));
  else envelope(s,ch,op);
  if(group==0xa0u)lfo(s,ch);
 }else if(reg==0x18||reg==0x19||reg==0x1b){
  if(reg==0x19){if(value&128u)s->pmd=value&127u;else s->amd=value;}
  for(unsigned ch=0;ch<8;ch++)lfo(s,ch);
 }else if((reg==0x0f&&(value&128u))||(reg==0x14&&(value&128u))||(reg==1&&value)){
  /* These modes are absent from the audited catalogue; never silently drop. */
  s->fault=1;
 }
 return !s->fault;
}
