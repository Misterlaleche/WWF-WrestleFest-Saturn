/* Screenshot fixture only. Execute the unchanged no-input gameplay path,
 * then stop after the selected ACK; do not edit actors, energy or outcomes. */
#include "wf_runtime.h"
static wf_status_t capture_frame(wf_runtime_t *);
#define wf_runtime_frame capture_frame
#include "../../src/wf_saturn_entry.c"
#undef wf_runtime_frame
#ifndef WF_R18_PROFILE_STOP
#define WF_R18_PROFILE_STOP 600u
#endif
static wf_status_t capture_frame(wf_runtime_t *r)
{
    wf_status_t status = wf_runtime_frame(r);
    if (r->g.frame_counter == WF_R18_PROFILE_STOP)
        for (;;) smpc_peripheral_process();
    return status;
}
