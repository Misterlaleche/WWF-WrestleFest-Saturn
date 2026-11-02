#ifndef WF_MUSIC_SEQ_H
#define WF_MUSIC_SEQ_H
#include <stddef.h>
#include "reference_driver/z80.h"
enum {WF_MUSIC_YM=0,WF_MUSIC_OKI=1,WF_MUSIC_BANK=2,WF_MUSIC_IRQ=3};
typedef bool (*wf_music_emit_fn)(unsigned kind,unsigned reg,unsigned value,uint64_t cycle,void *);
typedef struct {
 z80 cpu;
 const uint8_t *rom;
 uint8_t ram[2048],ym_addr,status,tb,ctl,latch,bank;
 uint32_t next_tb,last_cpu;
 uint64_t cycles,steps,skipped;
 wf_music_emit_fn emit;void *user;
 bool optimized,error,command_pending;
 /* Appended activity metadata only: preserve every pre-existing field offset. */
 uint32_t io_epoch;
} wf_music_seq_t;
bool wf_music_seq_init(wf_music_seq_t *,const uint8_t *,size_t,wf_music_emit_fn,void *,bool optimized);
bool wf_music_seq_send(wf_music_seq_t *,uint8_t);
/* 0 reached target, 1 budget exhausted, -1 fault. No PCM/OPM synthesis. */
int wf_music_seq_run(wf_music_seq_t *,uint64_t target,unsigned instruction_budget);
#endif
