#ifndef WF_R18_REFEREE_H
#define WF_R18_REFEREE_H
#include "wf_r18_video.h"
/* Adapter for $1FCC4..$1FCE6: target +$60/+62 -> angle +$2D.
 * The scratch area $1C15E4 is internal; D0-D7 remain unchanged. */
wf_status_t wf_r18_referee_direction_20c8(wf_runtime_t*,wf_actor_raw_t*,void*);
/* Exact $2067C count tiles. Number 0..20; bit15 selects the lower window.
 * user is wf_r18_video_t with the canonical ROM reader. */
wf_status_t wf_r18_referee_number_2067c(wf_runtime_t*,uint16_t,void*);
/* Pin-count callback derives D0's input from referee +$24 >> 1. */
wf_status_t wf_r18_referee_display_2067c(wf_runtime_t*,void*);
wf_status_t wf_r18_referee_clear_206fe(wf_runtime_t*,void*);
#endif
