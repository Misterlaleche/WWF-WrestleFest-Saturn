#include "wf_r18_scsp.h"
#include "wf_r18_scsp_runtime.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static wf_native_host_t host;
static wf_native_mail_t mail;
static wf_music_module_shared_t shared;
static volatile uint32_t frames;
static unsigned writes,mutes,ticks;
static bool owner(void *u){(void)u;return true;}
static void barrier(void *u){(void)u;}
static void write16(uint32_t address,uint16_t value,void *u){
 (void)u;++writes;
 if(address==0x100400u){assert(value==0x0200u);++mutes;}
}
void wf_native_host_pcm_tick(wf_native_host_t *h){if(h&&h->ready&&!h->poisoned)++ticks;}
static void setup(bool music){
 memset(&host,0,sizeof host);memset(&mail,0,sizeof mail);memset(&shared,0,sizeof shared);
 mail.abi=WF_NATIVE_ABI;mail.state=WF_NATIVE_READY;host.mail=&mail;host.ready=true;
 host.ops.enter_owner=owner;host.ops.leave_owner=barrier;host.ops.barrier=barrier;host.ops.write16=write16;
 assert(wf_native_queue_init(&wf_r18_scsp_queue,&host));
 frames=100;shared.state=WF_MUSIC_RUNNING;shared.worker_frame=shared.dsp_progress_frame=(uint16_t)frames;
 wf_r18_scsp_runtime_init(&host,&shared,&frames,music);writes=mutes=ticks=0;
}
int main(void){
 setup(true);shared.state=WF_MUSIC_FAULT;
 assert(wf_r18_scsp_service());assert(wf_r18_scsp_diagnostics.disabled);
 assert(wf_r18_scsp_diagnostics.reason==WF_R18_AUDIO_MUSIC_STATE&&mutes==1&&shared.shutdown);
 unsigned count=writes;assert(wf_r18_scsp_audio(NULL,1,NULL)==WF_OK);assert(wf_r18_scsp_service());assert(writes==count);
 setup(false);for(unsigned i=0;i<64;i++)assert(wf_r18_scsp_audio(NULL,0x20,NULL)==WF_OK);
 assert(wf_r18_scsp_audio(NULL,0x21,NULL)==WF_OK);assert(wf_r18_scsp_queue.used==64);
 assert(wf_r18_scsp_diagnostics.dropped_sfx==1&&!wf_r18_scsp_diagnostics.disabled);
 assert(wf_r18_scsp_audio(NULL,0,NULL)==WF_OK);assert(mutes==1&&wf_r18_scsp_diagnostics.disabled);
 assert(wf_r18_scsp_diagnostics.reason==WF_R18_AUDIO_STOP_PRESSURE);
 setup(true);shared.producer=64;shared.consumer=0;
 assert(wf_r18_scsp_audio(NULL,1,NULL)==WF_OK);assert(shared.producer==64);
 assert(wf_r18_scsp_diagnostics.dropped_music==1&&!wf_r18_scsp_diagnostics.disabled);
 assert(wf_r18_scsp_audio(NULL,0,NULL)==WF_OK);assert(shared.shutdown&&mutes==1);
 setup(false);assert(wf_r18_scsp_audio(NULL,0x20,NULL)==WF_OK);assert(wf_r18_scsp_service());
 uint16_t request=mail.request;frames+=6;assert(wf_r18_scsp_service());
 assert(host.poisoned&&wf_r18_scsp_queue.fault&&mutes==1);
 mail.ack=request;assert(wf_r18_scsp_audio(NULL,0x20,NULL)==WF_OK);assert(wf_r18_scsp_service());assert(mail.request==request);
 setup(true);frames+=31;assert(wf_r18_scsp_service());
 assert(wf_r18_scsp_diagnostics.reason==WF_R18_AUDIO_WORKER_STALL&&mutes==1);
 setup(true);frames+=31;shared.worker_frame=(uint16_t)frames;assert(wf_r18_scsp_service());
 assert(wf_r18_scsp_diagnostics.reason==WF_R18_AUDIO_DSP_STALL&&mutes==1);
 setup(true);for(unsigned i=0;i<120;i++){
  ++frames;shared.worker_frame=shared.dsp_progress_frame=(uint16_t)frames;
  wf_r18_scsp_vblank();assert(shared.master_frame==(uint16_t)frames);assert(wf_r18_scsp_service());
 }
 assert(!wf_r18_scsp_diagnostics.disabled&&mutes==0&&ticks==120);
 setup(true);frames=5;shared.worker_frame=shared.dsp_progress_frame=0xfffau;
 assert(wf_r18_scsp_service());assert(!wf_r18_scsp_diagnostics.disabled);
 setup(true);assert(wf_r18_scsp_audio(NULL,0x20,NULL)==WF_OK);
 assert(wf_r18_scsp_audio(NULL,0,NULL)==WF_OK);assert(shared.producer==1&&shared.commands[0]==0);
 assert(wf_r18_scsp_audio(NULL,1,NULL)==WF_OK);assert(shared.producer==2&&shared.commands[1]==1);
 assert(wf_r18_scsp_queue.used==2&&wf_r18_scsp_queue.words[0]==0x20&&wf_r18_scsp_queue.words[1]==0);
 setup(true);shared.worker_frame=shared.dsp_progress_frame=(uint16_t)(frames+1u);
 assert(wf_r18_scsp_service());
 if(wf_r18_scsp_diagnostics.disabled){puts("RED: worker stamp one frame ahead causes a false watchdog fault");return 1;}
 puts("PASS runtime audio containment: faults, saturation, stop, ack poison, heartbeat, wrap, ordering");
 return 0;
}
