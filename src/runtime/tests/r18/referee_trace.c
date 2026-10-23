/* Read-only observation around the real referee owner. */
#define wf_r17_referee_logic_1f914 referee_unobserved_1f914
#include "../../src/wf_r17_referee.c"
#undef wf_r17_referee_logic_1f914
#include "allies_trace.h"
wf_status_t wf_r17_referee_logic_1f914(wf_runtime_t *r,wf_actor_raw_t *a,void *user)
{
    allies_trace_begin(r,0u,3u,0x1f914u);
    wf_status_t status=referee_unobserved_1f914(r,a,user);
    allies_trace_end(r,0u,3u,0x1f914u,status);
    return status;
}
