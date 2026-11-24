#ifndef WF_R18_SCSP_H
#define WF_R18_SCSP_H
#ifdef WF_JO_ENGINE
#include "wf_jo_audio.h"
#else
#include "wf_runtime.h"
#include "../../audio/native/wf_native_queue.h"
bool wf_r18_scsp_boot(volatile uint32_t *video_clock,const void *elf,size_t bytes);
bool wf_r18_scsp_smpc_owned(void);
void wf_r18_scsp_vblank(void);
bool wf_r18_scsp_service(void);
wf_status_t wf_r18_scsp_audio(wf_runtime_t *,uint16_t raw,void *);
extern wf_native_queue_t wf_r18_scsp_queue; /* debugger-visible evidence */
#endif
#endif
