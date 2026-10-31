#include "wf_music_trace.h"
#include <string.h>
static uint16_t be16(const uint8_t *p)
{
#ifdef __sh__
 return *(const uint16_t *)(const void *)p;
#else
 return (uint16_t)(p[0]*256u+p[1]);
#endif
}
static uint32_t be32(const uint8_t *p){return (uint32_t)be16(p)*65536u+be16(p+2);}
static uint64_t be64(const uint8_t *p){return ((uint64_t)be32(p)<<32)|be32(p+4);}
static bool byte(wf_music_trace_t *s,uint8_t *value)
{
 uint16_t symbol;
 if(s->at.depth)symbol=s->at.stack[--s->at.depth];
 else{
  if(s->at.root>=s->root_count)return false;
  symbol=be16(s->roots+2u*s->at.root++);
 }
 while(symbol>=256){
  if((unsigned)symbol-256u>=s->rule_count||s->at.depth>=WF_TRACE_STACK)return false;
  const uint8_t *rule=s->rules+4u*((unsigned)symbol-256u);
  uint16_t left=be16(rule),right=be16(rule+2);
  if(left>=symbol||right>=symbol)return false;
  s->at.stack[s->at.depth++]=right;symbol=left;
 }
 *value=(uint8_t)symbol;return true;
}
/* Lift of the original ROM's empty command/timer poll. At PC003A a
 * zero-status lap is120T; the C801 load observes the timer state after79T.
 * PC003D omits the first62T command-queue call. Poll success reaches0409.
 * Every work token between polls was compared against the original core. */
static bool poll(wf_music_trace_t *s,bool partial)
{
 wf_music_trace_clock_t *t=s->clock;uint64_t at=s->previous;
 if(!t||(t->status&~2u)||!(t->ctl&2u)||at>UINT64_MAX-512u)return false;
 wf_music_trace_clock_advance(t,at);
 if(partial){
  if((t->status&2u)||t->next<=at+17u){at+=80u;goto complete;}
  at+=58u;wf_music_trace_clock_advance(t,at);
 }
 if(!(t->status&2u)&&t->next>at+79u){
  uint64_t n=(t->next-at-79u+119u)/120u;
  if(n>(UINT64_MAX-at-142u)/120u)return false;
  at+=n*120u;
 }
 at+=142u;
complete:
 wf_music_trace_clock_advance(t,at);s->previous=at;return true;
}
static bool next(wf_music_trace_t *s)
{
 unsigned polls=0;
again:
 if(s->record==s->prefix_count&&!s->loop_saved){s->loop_at=s->at;s->loop_saved=true;}
 if(s->record==s->prefix_count+s->loop_count){
  if(!s->loop_saved)return false;
  s->record=s->prefix_count;s->at=s->loop_at;
  if(!s->clock){
   if(s->loop_offset>UINT64_MAX-s->period)return false;
   s->loop_offset+=s->period;
   if(s->loop_before>UINT64_MAX-s->loop_offset)return false;
   s->previous=s->loop_before+s->loop_offset;
  }
 }
 uint32_t delta=0;unsigned shift=0;uint8_t b;
 do{
  if(!byte(s,&b)||shift>28u||(shift==28u&&(b&0xf0u)))return false;
  delta|=(uint32_t)(b&127u)<<shift;shift+=7u;
 }while(b&128u);
 if(!byte(s,&s->kind)||!byte(s,&s->reg)||!byte(s,&s->value)||s->previous>UINT64_MAX-delta)return false;
 s->previous+=delta;++s->record;
 if(s->kind>=4u){
  if(++polls>1024u||s->kind>5u||s->reg||s->value||!poll(s,s->kind==5u))return false;
  goto again;
 }
 if(s->kind>WF_MUSIC_BANK)return false;
 s->next_cycle=s->previous;return true;
}
static bool open_trace(wf_music_trace_t *s,const void *data,size_t bytes,unsigned command,
                       wf_music_trace_clock_t *clock,uint64_t epoch)
{
 const uint8_t *p=data;
 if(!s||!p||bytes<80||bytes>275808u||((uintptr_t)p&1u)||be32(p)!=(clock?0x57465031u:0x57465431u)||
    be16(p+4)!=1||be16(p+6)!=64||be32(p+8)!=bytes||be32(p+12)!=command||be32(p+60)||
    be32(p+64)!=0x52505231u||be32(p+48)!=be32(p+68)||be32(p+52)!=be32(p+72)||be32(p+56)!=be32(p+76))return false;
 uint32_t rules=be32(p+52),roots=be32(p+56),prefix=be32(p+24),loop=be32(p+28);
 if(rules>65280u||!roots||roots>bytes/2u||80u+rules*4u+roots*2u!=bytes||
    !loop||prefix>UINT32_MAX-loop||prefix+loop>be32(p+48)/4u||!be64(p+32))return false;
 if(clock&&(be64(p+16)||be64(p+32)!=1u||be64(p+40)))return false;
 memset(s,0,sizeof *s);
 s->rules=p+80;s->roots=s->rules+rules*4u;s->rule_count=rules;s->root_count=roots;
 s->prefix_count=prefix;s->loop_count=loop;s->period=be64(p+32);s->loop_before=be64(p+40);
 s->clock=clock;s->boot_cycle=clock?epoch:be64(p+16);s->previous=s->boot_cycle;
 if((!clock&&s->loop_before<s->boot_cycle)||!next(s)){s->error=true;return false;}
 return true;
}
bool wf_music_trace_open(wf_music_trace_t *s,const void *data,size_t bytes,unsigned command)
{return open_trace(s,data,bytes,command,NULL,0);}
bool wf_music_trace_open_phase(wf_music_trace_t *s,const void *data,size_t bytes,unsigned command,
                              wf_music_trace_clock_t *clock,uint64_t epoch)
{
 if(!clock||(command!=5u&&command!=9u))return false;
 return open_trace(s,data,bytes,command,clock,epoch);
}
int wf_music_trace_run(wf_music_trace_t *s,uint64_t target,unsigned budget,wf_music_emit_fn emit,void *user)
{
 if(!s||s->error||!emit)return -1;
 while(s->next_cycle<=target&&budget--){
  if(!emit(s->kind,s->reg,s->value,s->next_cycle,user)||!next(s)){s->error=true;return -1;}
  ++s->events;
 }
 return s->next_cycle<=target?1:0;
}
