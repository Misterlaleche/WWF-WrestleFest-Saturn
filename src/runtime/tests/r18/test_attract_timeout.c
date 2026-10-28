#include <assert.h>
#include <string.h>
#include "wf_r18_match_result.h"

int main(void)
{
    wf_runtime_t runtime;
    memset(&runtime, 0, sizeof runtime);
    runtime.frame_stage = 2;
    runtime.frame_ready = runtime.vblank_ack = 1;
    runtime.g.global_0166 = 0;
    runtime.g.frame_counter = 0x350;
    assert(!wf_r18_attract_timeout(&runtime));
    runtime.g.frame_counter = 0x351;
    assert(wf_r18_attract_timeout(&runtime));
    runtime.g.credited_route = 1;
    assert(!wf_r18_attract_timeout(&runtime));
    runtime.g.credited_route = 0;
    runtime.frame_stage = 1;
    assert(!wf_r18_attract_timeout(&runtime));
    runtime.frame_stage = 2;
    runtime.g.global_0166 = 13;
    runtime.g.frame_counter = 0x361;
    assert(wf_r18_attract_timeout(&runtime));
    runtime.g.global_0166 = 14;
    assert(!wf_r18_attract_timeout(&runtime));
    return 0;
}
