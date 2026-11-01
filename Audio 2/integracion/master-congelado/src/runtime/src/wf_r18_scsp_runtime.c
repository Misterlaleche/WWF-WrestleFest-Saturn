/* Audio admission/failure policy: never wait or return an audio fault to gameplay. */
#include "wf_r18_scsp.h"
#include "wf_r18_scsp_runtime.h"
#include <string.h>
wf_native_queue_t wf_r18_scsp_queue;
wf_r18_scsp_diagnostics_t wf_r18_scsp_diagnostics;
static wf_native_host_t *transport;
static volatile wf_music_module_shared_t *worker;
static volatile uint32_t *video_frames;
static bool native_music;
void wf_r18_scsp_runtime_init(wf_native_host_t *h,volatile wf_music_module_shared_t *m,
                              volatile uint32_t *frames,bool music)
{
 transport=h;worker=m;video_frames=frames;native_music=music;
 memset(&wf_r18_scsp_diagnostics,0,sizeof wf_r18_scsp_diagnostics);
}
void wf_r18_scsp_disable(unsigned reason)
{
 if(wf_r18_scsp_diagnostics.disabled)return;
 wf_r18_scsp_diagnostics.reason=reason;wf_r18_scsp_diagnostics.disabled=true;
 if(native_music&&worker)worker->shutdown=1;
 wf_r18_scsp_queue.fault=true;
 if(transport){
  transport->poisoned=true;
  /* Fixed SCSP master mute, no mailbox transaction or SMPC request. Keep
   * MEM4MB configured for the 512 KiB sound RAM. A late worker cannot undo it. */
  if(transport->ops.write16)transport->ops.write16(0x100400u,0x0200u,transport->ops.user);
  if(transport->ops.barrier)transport->ops.barrier(transport->ops.user);
 }
}
void wf_r18_scsp_vblank(void)
{
 if(wf_r18_scsp_diagnostics.disabled)return;
 if(native_music&&worker&&video_frames)worker->master_frame=(uint16_t)*video_frames;
 if(transport)wf_native_host_pcm_tick(transport);
}
bool wf_r18_scsp_service(void)
{
 if(wf_r18_scsp_diagnostics.disabled||!transport||!video_frames)return true;
 if(native_music){
  unsigned reason=0;uint16_t frame=(uint16_t)*video_frames;
  if(worker->state!=WF_MUSIC_RUNNING)reason=WF_R18_AUDIO_MUSIC_STATE;
  else if((int16_t)(uint16_t)(frame-worker->worker_frame)>30)reason=WF_R18_AUDIO_WORKER_STALL;
  else if((int16_t)(uint16_t)(frame-worker->dsp_progress_frame)>30)reason=WF_R18_AUDIO_DSP_STALL;
  if(reason){wf_r18_scsp_disable(reason);return true;}
 }
 if(!wf_native_queue_service(&wf_r18_scsp_queue,*video_frames*16667u))
  wf_r18_scsp_disable(WF_R18_AUDIO_TRANSPORT);
 return true;
}
wf_status_t wf_r18_scsp_audio(wf_runtime_t *rt,uint16_t raw,void *user)
{
 (void)rt;(void)user;unsigned command=raw&255u;
 if(wf_r18_scsp_diagnostics.disabled||!transport){++wf_r18_scsp_diagnostics.ignored;return WF_OK;}
 if(native_music&&(!command||(command>=1&&command<=18)||command==0x1f)){
  if(worker->state!=WF_MUSIC_RUNNING){wf_r18_scsp_disable(WF_R18_AUDIO_MUSIC_STATE);return WF_OK;}
  uint16_t producer=worker->producer;
  if((uint16_t)(producer-worker->consumer)>=64u){
   if(!command)wf_r18_scsp_disable(WF_R18_AUDIO_STOP_PRESSURE);
   else ++wf_r18_scsp_diagnostics.dropped_music;
   return WF_OK;
  }
  /* Stop is ordered in both streams. If it cannot be admitted, latch a
   * master mute rather than lose stop or rewrite a consumer-owned FIFO. */
  if(!command&&!wf_native_queue_push(&wf_r18_scsp_queue,raw)){
   wf_r18_scsp_disable(WF_R18_AUDIO_STOP_PRESSURE);return WF_OK;
  }
  worker->commands[producer&63u]=(uint16_t)command;
  (void)worker->commands[producer&63u];worker->producer=(uint16_t)(producer+1u);
  return WF_OK;
 }
 if(!wf_native_queue_push(&wf_r18_scsp_queue,raw)){
  if(wf_r18_scsp_queue.fault||transport->poisoned)wf_r18_scsp_disable(WF_R18_AUDIO_TRANSPORT);
  else if(!command)wf_r18_scsp_disable(WF_R18_AUDIO_STOP_PRESSURE);
  else ++wf_r18_scsp_diagnostics.dropped_sfx;
 }
 return WF_OK;
}
