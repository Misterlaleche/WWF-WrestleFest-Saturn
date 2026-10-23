/* Test-only observation of the real main/action owner. */
#define wf_r17_dispatch_1c03e_bounded allies_dispatch_unobserved
#include "../../src/wf_r17_dispatch_1c03e.c"
#undef wf_r17_dispatch_1c03e_bounded
#include "allies_trace.h"
wf_status_t wf_r17_dispatch_1c03e_bounded(wf_runtime_t *rt,wf_actor_raw_t *a,wf_r17_dispatch_1c03e_ctx_t *ctx){
 unsigned slot=(unsigned)(a-rt->actors);
 allies_trace_begin(rt,slot,1u,0x1c03eu);
 wf_status_t status=allies_dispatch_unobserved(rt,a,ctx);
 allies_trace_end(rt,slot,1u,0x1c03eu,status);
 return status;
}
