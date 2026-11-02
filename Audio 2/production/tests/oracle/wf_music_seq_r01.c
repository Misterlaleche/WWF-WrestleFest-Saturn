#include "wf_music_seq.h"
#include <string.h>
#ifdef WF_MUSIC_MODULE_TEST
extern void wf_music_test_latch(unsigned);
#endif
static uint64_t current(const wf_music_seq_t *v){return v->cycles+(uint32_t)((uint32_t)v->cpu.cyc-v->last_cpu);}
static void emit(wf_music_seq_t *v,unsigned kind,unsigned r,unsigned value)
{
 if(v->emit&&!v->emit(kind,r,value,current(v),v->user))v->error=true;
}
static uint8_t read_byte(void *user,uint16_t a)
{
 wf_music_seq_t *v=user;
 if(a<0xc000)return v->rom[a];
 if(a<0xc800)return v->ram[a-0xc000];
 if(a==0xc800||a==0xc801)return v->status;
 if(a==0xd800)return 0;
 if(a==0xe000){
#ifdef WF_MUSIC_MODULE_TEST
  wf_music_test_latch(v->latch);
#endif
  v->command_pending=false;return v->latch;
 }
 return 255;
}
static void write_byte(void *user,uint16_t a,uint8_t value)
{
 wf_music_seq_t *v=user;
 if(a>=0xc000&&a<0xc800){v->ram[a-0xc000]=value;return;}
 if(a==0xc800){v->ym_addr=value;return;}
 if(a==0xc801){
  emit(v,WF_MUSIC_YM,v->ym_addr,value);
  if(v->ym_addr==0x12)v->tb=value;
  if(v->ym_addr==0x14){
   uint8_t old=v->ctl;v->ctl=value;
   if(value&32)v->status&=0xfdu;
   if((value&2)&&!(old&2))v->next_tb=(uint32_t)v->cpu.cyc+(256u-v->tb)*1024u;
  }
 }else if(a==0xd800)emit(v,WF_MUSIC_OKI,0,value);
 else if(a==0xe800){v->bank=value&1u;emit(v,WF_MUSIC_BANK,0,value);}
}
static uint8_t port_in(z80 *z,uint8_t p){(void)z;(void)p;return 255;}
static void port_out(z80 *z,uint8_t p,uint8_t x){(void)z;(void)p;(void)x;}
bool wf_music_seq_init(wf_music_seq_t *v,const uint8_t *p,size_t n,wf_music_emit_fn e,void *u,bool optimized)
{
 static const uint8_t loop[]={0xcd,0x7a,0,0xcd,0xe0,3,0xc3,0x3a,0};
 if(!v||!p||n<0xc000||memcmp(p+0x3a,loop,sizeof loop))return false;
 memset(v,0,sizeof *v);v->rom=p;v->emit=e;v->user=u;v->optimized=optimized;
 z80_init(&v->cpu);v->cpu.userdata=v;v->cpu.read_byte=read_byte;v->cpu.write_byte=write_byte;
 v->cpu.port_in=port_in;v->cpu.port_out=port_out;return true;
}
bool wf_music_seq_send(wf_music_seq_t *v,uint8_t cmd)
{
 if(!v||v->error||v->command_pending||!((cmd>=1&&cmd<=18)||cmd==0x1f))return false;
 v->latch=cmd;v->command_pending=true;z80_gen_nmi(&v->cpu);return true;
}
static void update_timer(wf_music_seq_t *v)
{
 uint32_t now=(uint32_t)v->cpu.cyc;
 if((v->ctl&2)&&(int32_t)(now-v->next_tb)>=0){
  uint32_t period=(256u-v->tb)*1024u;
  do{v->next_tb+=period;}while((int32_t)(now-v->next_tb)>=0);
  v->status|=2;
  if(v->ctl&8){z80_gen_int(&v->cpu,255);emit(v,WF_MUSIC_IRQ,0,255);}
 }
}
static bool idle_skip(wf_music_seq_t *v,uint64_t target)
{
 z80 *z=&v->cpu;uint64_t available=target-v->cycles;uint32_t n,cycles;
 /* Exact lifted 120-T-state empty poll loop. No RAM/I-O write can occur;
  * two CALL stack stores already contain their final0x0040 return address.
  * Never skip across a timer boundary, pending command or accepted IRQ. */
 if(z->pc!=0x3a||z->nmi_pending||(z->int_pending&&z->iff1)||z->iff_delay||
    (v->status&3)||v->ram[2]!=v->ram[3]||z->a||z->b!=v->ram[2]||
    !z->zf||!z->hf||!z->pf||z->cf||z->nf||z->sf||z->xf||z->yf||z->mem_ptr!=0x3a||
    z->sp<0xc002||z->sp>0xc800||v->ram[z->sp-0xc002]!=0x40||v->ram[z->sp-0xc001]!=0)return false;
 if(v->ctl&2){
  int32_t until=(int32_t)(v->next_tb-(uint32_t)z->cyc);
  if(until<=0)return false;
  if(available>(uint32_t)until)available=(uint32_t)until;
 }
 if(available<=120)return false;
 n=(uint32_t)((available-1)/120u);cycles=n*120u;
 z->cyc+=cycles;z->r=(uint8_t)((z->r&128u)|((z->r+n*11u)&127u));
 v->last_cpu=(uint32_t)z->cyc;v->cycles+=cycles;v->skipped+=(uint64_t)n*11u;return true;
}
int wf_music_seq_run(wf_music_seq_t *v,uint64_t target,unsigned budget)
{
 if(!v||v->error)return -1;
 while(v->cycles<target&&budget--){
  if(v->optimized&&idle_skip(v,target))continue;
  uint32_t before=(uint32_t)v->cpu.cyc;
  z80_step(&v->cpu);++v->steps;
  uint32_t elapsed=(uint32_t)((uint32_t)v->cpu.cyc-before);
  if(!elapsed||elapsed>64){v->error=true;break;}
  update_timer(v);v->cycles+=elapsed;v->last_cpu=(uint32_t)v->cpu.cyc;
  if(v->error)break;
 }
 return v->error?-1:(v->cycles<target?1:0);
}
