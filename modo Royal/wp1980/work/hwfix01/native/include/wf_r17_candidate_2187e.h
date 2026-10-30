#ifndef WF_R17_CANDIDATE_2187E_H
#define WF_R17_CANDIDATE_2187E_H
#include "wf_runtime.h"
/* Original Royal scan. All shadow D registers survive. found is the
 * original Z result; candidate is A1 (the scan end address on failure). */
wf_status_t wf_r17_candidate_2187e(wf_runtime_t *rt,unsigned slot,
    uint32_t *candidate,int *found,uint32_t *blocked_dependency);
#endif
