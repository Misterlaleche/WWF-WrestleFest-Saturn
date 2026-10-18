#ifndef WF_R17_POSTACK_2017A_H
#define WF_R17_POSTACK_2017A_H
#include "wf_r17_referee.h"
/* $206FE owns 4 x 16 zero bytes at $C0744 + n*$100. The platform
 * callback must implement that transfer; the MC68000 D registers survive it. */
wf_status_t wf_r17_postack_2017a(wf_runtime_t *rt,wf_r17_ref_simple_fn clear_206fe,void *user);
#endif
