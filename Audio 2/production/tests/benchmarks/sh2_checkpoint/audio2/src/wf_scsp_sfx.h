#ifndef WF_SCSP_SFX_H
#define WF_SCSP_SFX_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
enum {
    WF_SFX_PACK_BASE=0x14000,WF_SFX_PACK_CAPACITY=0x6c000,
    WF_SFX_PCM_BASE=0x3000,WF_SFX_RING_FRAMES=4096,WF_SFX_PCM_VOICES=4,
    WF_SFX_FIRST_SLOT=4,WF_SFX_CLOCK_SLOT=30
};
typedef bool (*wf_sfx_write16_fn)(uint32_t,uint16_t,void *);
/* Optional monotonic SCSP-frame clock, on the same epoch as s.frames.
 * It must not call advance or reenter the sequencer. */
typedef uint32_t (*wf_sfx_now_fn)(void *);
typedef bool (*wf_sfx_pcm_control_fn)(unsigned voice,bool start,unsigned volume,void *);
typedef struct {
    uint32_t offset,remaining;
    int16_t signal,index;
    uint8_t low;
} wf_sfx_decoder_t;
/* Pure initial waveform cache. It owns no voice and never keys hardware on. */
typedef struct {
    wf_sfx_decoder_t decoder;
    const uint8_t *pack;
    uint32_t total_frames;
    uint8_t bank,id;
    int16_t samples[256];
} wf_sfx_prepared_t;
typedef struct {
    uint32_t played,phase,write_at,write_end,last_frame,total_frames;
    wf_sfx_decoder_t decoder;
    uint8_t playing,suppress,initial_fill,sample_id,bank,volume;
} wf_scsp_sfx_pcm_t;
typedef struct {
    uint8_t active,command,event_index,ticks_remaining,priority,bank;
} wf_scsp_sfx_sequence_t;
typedef struct {
    const uint8_t *pack;
    uint32_t bytes,samples,commands,events,raw,timer_phase,ticks,underruns,frames;
    wf_scsp_sfx_pcm_t pcm[4];
    wf_scsp_sfx_sequence_t sequence[3];
    wf_sfx_write16_fn write16;
    wf_sfx_now_fn now;
    wf_sfx_pcm_control_fn pcm_control;
    uint32_t pcm_stride,start_guard;
    void *user;
    uint8_t bank,bound,fault,work_voice;
} wf_scsp_sfx_t;
bool wf_scsp_sfx_pack_valid(const uint8_t *,size_t);
void wf_scsp_sfx_init(wf_scsp_sfx_t *,wf_sfx_write16_fn,void *);
bool wf_scsp_sfx_configure_pcm(wf_scsp_sfx_t *,unsigned stride,unsigned guard,wf_sfx_pcm_control_fn);
bool wf_scsp_sfx_bind(wf_scsp_sfx_t *,const uint8_t *,size_t);
int wf_scsp_sfx_command(wf_scsp_sfx_t *,uint8_t); /* >=0 accepted, -1 priority, -2 error */
bool wf_scsp_sfx_oki_start(wf_scsp_sfx_t *,unsigned,unsigned,unsigned,unsigned,bool);
bool wf_scsp_sfx_prepare(const wf_scsp_sfx_t *,wf_sfx_prepared_t *,unsigned,unsigned);
/* Pure bounded preparation. Set *filled=0 to begin, then preserve it and the
 * cache between calls. Each successful call decodes at most budget samples,
 * up to the existing256-sample cache. Only *filled==256 is ready to launch. */
bool wf_scsp_sfx_prepare_step(const wf_scsp_sfx_t *,wf_sfx_prepared_t *,unsigned,unsigned,uint16_t *filled,unsigned budget);
bool wf_scsp_sfx_oki_start_prepared(wf_scsp_sfx_t *,unsigned,unsigned,unsigned,unsigned,bool,const wf_sfx_prepared_t *);
void wf_scsp_sfx_stop(wf_scsp_sfx_t *,unsigned);
void wf_scsp_sfx_stop_all(wf_scsp_sfx_t *);
void wf_scsp_sfx_advance(wf_scsp_sfx_t *,unsigned elapsed_scsp_frames);
void wf_scsp_sfx_work(wf_scsp_sfx_t *); /* <=128 samples globally, round-robin */
int16_t wf_scsp_sfx_decode(wf_sfx_decoder_t *,const uint8_t *);
#endif
