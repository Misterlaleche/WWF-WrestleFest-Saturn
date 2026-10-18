#ifndef WF_R17_TRANSIENT_RESET_10E6A_H
#define WF_R17_TRANSIENT_RESET_10E6A_H
#include "wf_runtime.h"

#define WF_R17_TRANSIENT_RECORDS 12u
#define WF_R17_TRANSIENT_STRIDE  0x2au

/* Exact owner at arcade $010E6A: clears only bit7 of byte +$00 in each
 * of the 12 transient records rooted at $1C1258, stride $2A. */
wf_status_t wf_r17_transient_reset_10e6a(wf_runtime_t *rt);
#endif
