#include <assert.h>
#include <stdint.h>
#include "wf24_perf_probe.h"
typedef struct { uint64_t tick; } fake_clock_t;
static uint64_t read_tick(void *u) { return ((fake_clock_t *)u)->tick; }
int main(void)
{
    fake_clock_t c = {100u};
    wf24_perf_probe_t p;
    assert(wf24_perf_probe_init(&p, read_tick, &c));
    assert(wf24_perf_frame_begin(&p));
    assert(wf24_perf_region_begin(&p, WF24_PERF_LIVE_RENDER));
    c.tick += 25u;
    assert(wf24_perf_region_end(&p, WF24_PERF_LIVE_RENDER));
    wf24_perf_add_rendered_samples(&p, 16384u);
    wf24_perf_add_scsp_copy(&p, 32768u);
    wf24_perf_count_refill(&p);
    c.tick += 15u;
    assert(wf24_perf_frame_end(&p));
    assert(p.region[WF24_PERF_LIVE_RENDER].max_ticks == 25u);
    assert(p.frame_max_ticks == 40u);
    assert(p.rendered_samples == 16384u && p.scsp_copy_bytes == 32768u);
    assert(p.refills == 1u);
    return 0;
}
