/* Bound only the next owner: all state9 decisions execute production CPU code. */
#include "test_tag_campaign_cpu.c"
wf_status_t __wrap_wf_state5_execute(wf_runtime_t *rt,unsigned slot,wf_state5_trace_t *trace)
{
    (void)rt;(void)trace;
    printf("boundary=1dfe2 slot=%u\n",slot);
    return WF_OK;
}
