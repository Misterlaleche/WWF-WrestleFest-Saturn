#ifndef WF_AUDIO_SFX_H
#define WF_AUDIO_SFX_H
#include <stdint.h>
#include <stdbool.h>
#define WF_SFX_FIRST_COMMAND 0x20u
#define WF_SFX_LAST_COMMAND 0x81u
#define WF_SFX_EVENT_MAX 9u
#define WF_SFX_TIMER_A_HZ_NUM 3579545u
#define WF_SFX_TIMER_A_HZ_DEN (64u*253u)
typedef struct {
 uint8_t sample_id,delay_ticks,volume_nibble,flags;
} wf_sfx_event_t;
typedef struct {
 uint8_t priority,bank_policy,event_count,cleanup_stop;
 wf_sfx_event_t events[WF_SFX_EVENT_MAX];
} wf_sfx_command_t;
typedef struct {
 bool active;
 uint8_t command,event_index,ticks_remaining,priority,bank;
} wf_sfx_voice_t;
typedef void (*wf_sfx_play_fn)(unsigned logical_voice,unsigned bank,unsigned sample,
                               unsigned volume_nibble,bool suppress_stop,void *user);
typedef void (*wf_sfx_stop_fn)(unsigned logical_voice,void *user);
extern const wf_sfx_command_t wf_sfx_commands[98];
const wf_sfx_command_t *wf_sfx_command(unsigned command);
void wf_sfx_runtime_reset(void);
int wf_sfx_start_voice(unsigned command,wf_sfx_play_fn play,wf_sfx_stop_fn stop,void *user);
bool wf_sfx_start(unsigned command,wf_sfx_play_fn play,wf_sfx_stop_fn stop,void *user);
void wf_sfx_timer_a_tick(wf_sfx_play_fn play,wf_sfx_stop_fn stop,void *user);
const wf_sfx_voice_t *wf_sfx_voice(unsigned index);
#endif
