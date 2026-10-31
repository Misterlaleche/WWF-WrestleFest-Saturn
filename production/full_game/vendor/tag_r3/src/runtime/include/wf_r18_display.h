#ifndef WF_R18_DISPLAY_H
#define WF_R18_DISPLAY_H
#include "wf_r18_video.h"
typedef struct wf_r18_display {
 wf_r18_rom_read_fn read_rom;void *user;
 uint16_t shake[15],scroll[4];
} wf_r18_display_t;
wf_status_t wf_r18_display_init(wf_r18_display_t*,const wf_runtime_t*,wf_r18_rom_read_fn,void*);
wf_status_t wf_r18_display_irq(wf_runtime_t*,wf_r18_display_t*);
#endif
