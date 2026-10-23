/* Observe only a real scene-transition request. No callback is substituted. */
#define wf_r17_f98c transition_unobserved_f98c
#include "../../src/wf_r17_dependencies.c"
#undef wf_r17_f98c
#include "allies_trace.h"
wf_status_t wf_r17_f98c(wf_runtime_t *r,wf_gameplay_binding_context_t *c)
{
    int requested=(r->g.global_1678&128u)!=0u;
    if(requested)allies_trace_begin(r,0u,2u,0xf98cu);
    wf_status_t status=transition_unobserved_f98c(r,c);
    if(requested)allies_trace_end(r,0u,2u,0xf98cu,status);
    return status;
}
