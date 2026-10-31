#ifndef WF_MUSIC_TRACE_CLOCK_H
#define WF_MUSIC_TRACE_CLOCK_H
#include "wf_music_seq.h"
/* Preserve the original YM Timer B phase while the ROM event grammar plays.
 * The track-4 trace contains one TB=237 write, and only ctl=47 acknowledgments;
 * this is verified from every event in two complete 1335.786s periods. */
typedef struct {uint64_t next;uint8_t tb,ctl,status;} wf_music_trace_clock_t;
static inline void wf_music_trace_clock_init(wf_music_trace_clock_t *t,const wf_music_seq_t *s)
{
 t->next=s->cycles+(uint32_t)(s->next_tb-(uint32_t)s->cpu.cyc);
 t->tb=s->tb;t->ctl=s->ctl;t->status=s->status;
}
static inline void wf_music_trace_clock_advance(wf_music_trace_clock_t *t,uint64_t cycle)
{
 if((t->ctl&2u)&&cycle>=t->next){
  uint32_t period=(256u-t->tb)*1024u;
  t->next+=((cycle-t->next)/period+1u)*period;t->status|=2u;
 }
}
static inline void wf_music_trace_clock_event(wf_music_trace_clock_t *t,unsigned kind,unsigned reg,unsigned value,uint64_t cycle)
{
 wf_music_trace_clock_advance(t,cycle);
 if(kind!=WF_MUSIC_YM)return;
 if(reg==0x12u)t->tb=(uint8_t)value;
 if(reg==0x14u){
  unsigned old=t->ctl;t->ctl=(uint8_t)value;
  if(value&32u)t->status&=(uint8_t)~2u;
  if((value&2u)&&!(old&2u))t->next=cycle+(256u-t->tb)*1024u;
 }
}
static inline void wf_music_trace_clock_resume(const wf_music_trace_clock_t *t,wf_music_seq_t *s,uint64_t cycle)
{
 s->cycles=cycle;s->cpu.cyc=(uint32_t)cycle;s->last_cpu=(uint32_t)cycle;
 s->next_tb=(uint32_t)t->next;s->tb=t->tb;s->ctl=t->ctl;s->status=t->status;
}
#endif
