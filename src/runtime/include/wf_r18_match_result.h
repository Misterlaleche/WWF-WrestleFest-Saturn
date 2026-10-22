#ifndef WF_R18_MATCH_RESULT_H
#define WF_R18_MATCH_RESULT_H
#include "wf_runtime.h"
wf_status_t wf_r18_tag_bonus_90d6(wf_runtime_t *,void *);
wf_status_t wf_r18_result_sound_20156(wf_runtime_t *,void *);
/* Observe the real $1178/$1186 -> $11B6 transition after frame ACK. */
int wf_r18_tag_match_complete(const wf_runtime_t *);
#endif
