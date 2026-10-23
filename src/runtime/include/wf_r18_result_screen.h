#ifndef WF_R18_RESULT_SCREEN_H
#define WF_R18_RESULT_SCREEN_H

#include "wf_r18_video.h"

/* Physical $21E6 boundary: count is D0.W+1 complete VBlank periods. */
typedef wf_status_t (*wf_r18_result_wait_fn)(wf_runtime_t *,uint16_t,void *);
/* Publish the FG0 changes already made by the real translated $2503C. */
typedef wf_status_t (*wf_r18_result_commit_fn)(void *);

/* Presentation segment $11CE..$122B only. This is not the complete $11B6
 * owner: scratch clears $0092..$009E and scene changes $122C+ are excluded.
 * Requires a valid register shadow, ROM reader and both physical callbacks.
 * Stops on the first failed wait, real text operation or physical commit;
 * a failed call is not a resumable continuation of this segment. */
wf_status_t wf_r18_result_in_ring(wf_runtime_t *,wf_r18_video_t *,
                                 wf_r18_result_wait_fn,
                                 wf_r18_result_commit_fn,void *);

#endif
