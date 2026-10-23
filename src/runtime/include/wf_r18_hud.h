#ifndef WF_R18_HUD_H
#define WF_R18_HUD_H
#include "wf_r18_video.h"
typedef int (*wf_r18_hud_file_fn)(const char*,uint32_t,void*,size_t);
wf_status_t wf_r18_hud_irq(wf_runtime_t*,wf_r18_video_t*);
void wf_r18_hud_reset(wf_r18_video_t*);
wf_status_t wf_r18_hud_map_update(wf_r18_video_t*,volatile uint16_t*,size_t*);
wf_status_t wf_r18_hud_bind(wf_r18_video_t*,wf_r18_hud_file_fn);
wf_status_t wf_r18_hud_commit(wf_r18_video_t*);
#endif
