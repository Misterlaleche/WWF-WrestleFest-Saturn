#ifndef WF_R18_MATCH_AUX_H
#define WF_R18_MATCH_AUX_H
#include "wf_r18_prematch.h"
#include "wf_r18_video.h"
/* Original $2208 raw movement, $B29E card text and $B2E4 text clear. The caller supplies
 * the native $9ACA dependency for B29E; no actor cast or clock is used. */
wf_status_t wf_r18_match_aux_call(wf_r18_prematch_t *,wf_r18_video_t *,uint32_t owner);
#endif
