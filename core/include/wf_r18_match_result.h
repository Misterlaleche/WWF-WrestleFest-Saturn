#ifndef WF_R18_MATCH_RESULT_H
#define WF_R18_MATCH_RESULT_H
#include "wf_runtime.h"
wf_status_t wf_r18_tag_bonus_90d6(wf_runtime_t *,void *);
wf_status_t wf_r18_result_sound_20156(wf_runtime_t *,void *);
/* Observe the real $1178/$1186 -> $11B6 transition after frame ACK. */
int wf_r18_tag_match_complete(const wf_runtime_t *);
/* Original $1108..$1146: an uncredited demo ends only after its ROM table
 * threshold, following the acknowledged gameplay frame. */
int wf_r18_attract_timeout(const wf_runtime_t *);
/* Observe the original $1160..$1170 -> $1AFC loss branch after ACK.
 * The high-byte winner branch to $1B4E has priority. */
int wf_r18_royal_game_over(const wf_runtime_t *);
#endif
