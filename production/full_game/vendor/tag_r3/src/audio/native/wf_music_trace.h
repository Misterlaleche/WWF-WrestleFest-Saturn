#ifndef WF_MUSIC_TRACE_H
#define WF_MUSIC_TRACE_H
#include "wf_music_seq.h"
#include "wf_music_trace_clock.h"
#define WF_TRACE_STACK 192u
typedef struct {
 uint32_t root;
 uint16_t depth,stack[WF_TRACE_STACK];
} wf_trace_position_t;
typedef struct {
 const uint8_t *rules,*roots;
 uint32_t rule_count,root_count,prefix_count,loop_count,record;
 uint64_t boot_cycle,period,loop_before,previous,loop_offset,next_cycle,events;
 wf_trace_position_t at,loop_at;
 uint8_t kind,reg,value;
 wf_music_trace_clock_t *clock;
 bool loop_saved,error;
} wf_music_trace_t;
bool wf_music_trace_open(wf_music_trace_t *,const void *,size_t,unsigned);
int wf_music_trace_run(wf_music_trace_t *,uint64_t,unsigned,wf_music_emit_fn,void *);
bool wf_music_trace_open_phase(wf_music_trace_t *,const void *,size_t,unsigned,wf_music_trace_clock_t *,uint64_t);
#endif
