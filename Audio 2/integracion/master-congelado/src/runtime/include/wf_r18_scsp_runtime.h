#ifndef WF_R18_SCSP_RUNTIME_H
#define WF_R18_SCSP_RUNTIME_H
#include "../../audio/native/wf_native_host.h"
#include "../../audio/native/wf_music_module.h"
enum {
 WF_R18_AUDIO_BOOT=1,WF_R18_AUDIO_MUSIC_STATE,WF_R18_AUDIO_TRANSPORT,
 WF_R18_AUDIO_WORKER_STALL,WF_R18_AUDIO_DSP_STALL,WF_R18_AUDIO_STOP_PRESSURE
};
typedef struct {
 uint32_t reason,dropped_sfx,dropped_music,ignored;
 bool disabled;
} wf_r18_scsp_diagnostics_t;
extern wf_r18_scsp_diagnostics_t wf_r18_scsp_diagnostics;
void wf_r18_scsp_runtime_init(wf_native_host_t *,volatile wf_music_module_shared_t *,volatile uint32_t *,bool);
/* Latches audio offline and mutes. Recovery requires a complete sound reboot. */
void wf_r18_scsp_disable(unsigned reason);
#endif
