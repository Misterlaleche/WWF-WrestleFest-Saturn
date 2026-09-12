#include "wf24_perf_probe.h"

#include <string.h>

static bool
region_valid(wf24_perf_region_t region)
{
    return (unsigned)region < (unsigned)WF24_PERF_REGION_COUNT;
}

bool
wf24_perf_probe_init(wf24_perf_probe_t *probe,
                     wf24_perf_read_ticks_fn read_ticks,
                     void *user)
{
    if (!probe || !read_ticks)
        return false;
    memset(probe, 0, sizeof(*probe));
    probe->read_ticks = read_ticks;
    probe->user = user;
    return true;
}

void
wf24_perf_probe_reset(wf24_perf_probe_t *probe)
{
    wf24_perf_read_ticks_fn read_ticks;
    void *user;
    if (!probe)
        return;
    read_ticks = probe->read_ticks;
    user = probe->user;
    memset(probe, 0, sizeof(*probe));
    probe->read_ticks = read_ticks;
    probe->user = user;
}

bool
wf24_perf_region_begin(wf24_perf_probe_t *probe, wf24_perf_region_t region)
{
    if (!probe || !probe->read_ticks || !region_valid(region) ||
        probe->region_active[region])
        return false;
    probe->region_start[region] = probe->read_ticks(probe->user);
    probe->region_active[region] = 1u;
    return true;
}

bool
wf24_perf_region_end(wf24_perf_probe_t *probe, wf24_perf_region_t region)
{
    uint64_t end, elapsed;
    wf24_perf_region_stats_t *s;
    if (!probe || !probe->read_ticks || !region_valid(region) ||
        !probe->region_active[region])
        return false;
    end = probe->read_ticks(probe->user);
    elapsed = end - probe->region_start[region];
    probe->region_active[region] = 0u;
    s = &probe->region[region];
    s->total_ticks += elapsed;
    if (s->calls == 0u || elapsed < s->min_ticks)
        s->min_ticks = elapsed;
    if (elapsed > s->max_ticks)
        s->max_ticks = elapsed;
    ++s->calls;
    return true;
}

bool
wf24_perf_frame_begin(wf24_perf_probe_t *probe)
{
    if (!probe || !probe->read_ticks || probe->frame_active)
        return false;
    probe->frame_start = probe->read_ticks(probe->user);
    probe->frame_active = 1u;
    return true;
}

bool
wf24_perf_frame_end(wf24_perf_probe_t *probe)
{
    uint64_t elapsed;
    if (!probe || !probe->read_ticks || !probe->frame_active)
        return false;
    elapsed = probe->read_ticks(probe->user) - probe->frame_start;
    probe->frame_active = 0u;
    probe->frame_total_ticks += elapsed;
    if (elapsed > probe->frame_max_ticks)
        probe->frame_max_ticks = elapsed;
    ++probe->frames;
    return true;
}

void wf24_perf_add_rendered_samples(wf24_perf_probe_t *p, uint32_t n)
{ if (p) p->rendered_samples += n; }
void wf24_perf_add_scsp_copy(wf24_perf_probe_t *p, uint32_t n)
{ if (p) p->scsp_copy_bytes += n; }
void wf24_perf_add_cdfs_read(wf24_perf_probe_t *p, uint32_t n)
{ if (p) p->cdfs_read_bytes += n; }
void wf24_perf_count_refill(wf24_perf_probe_t *p)
{ if (p) ++p->refills; }
void wf24_perf_count_underrun(wf24_perf_probe_t *p)
{ if (p) ++p->underruns; }
void wf24_perf_count_timer_a(wf24_perf_probe_t *p)
{ if (p) ++p->timer_a_ticks; }
void wf24_perf_count_dispatch(wf24_perf_probe_t *p)
{ if (p) ++p->dispatches; }
