#ifndef WF24_PERF_PROBE_H
#define WF24_PERF_PROBE_H

#include <stdbool.h>
#include <stdint.h>

typedef uint64_t (*wf24_perf_read_ticks_fn)(void *user);

typedef enum {
    WF24_PERF_LIVE_RENDER = 0,
    WF24_PERF_STREAM_SERVICE,
    WF24_PERF_SCSP_COPY,
    WF24_PERF_SFX_TIMER_A,
    WF24_PERF_COMMAND_DISPATCH,
    WF24_PERF_REGION_COUNT
} wf24_perf_region_t;

typedef struct {
    uint64_t total_ticks;
    uint64_t max_ticks;
    uint64_t min_ticks;
    uint32_t calls;
} wf24_perf_region_stats_t;

typedef struct {
    wf24_perf_read_ticks_fn read_ticks;
    void *user;
    wf24_perf_region_stats_t region[WF24_PERF_REGION_COUNT];
    uint64_t region_start[WF24_PERF_REGION_COUNT];
    uint8_t region_active[WF24_PERF_REGION_COUNT];
    uint64_t frame_start;
    uint64_t frame_total_ticks;
    uint64_t frame_max_ticks;
    uint32_t frames;
    uint64_t rendered_samples;
    uint64_t scsp_copy_bytes;
    uint64_t cdfs_read_bytes;
    uint32_t refills;
    uint32_t underruns;
    uint32_t timer_a_ticks;
    uint32_t dispatches;
    uint8_t frame_active;
} wf24_perf_probe_t;

bool wf24_perf_probe_init(wf24_perf_probe_t *probe,
                          wf24_perf_read_ticks_fn read_ticks,
                          void *user);
void wf24_perf_probe_reset(wf24_perf_probe_t *probe);
bool wf24_perf_region_begin(wf24_perf_probe_t *probe,
                            wf24_perf_region_t region);
bool wf24_perf_region_end(wf24_perf_probe_t *probe,
                          wf24_perf_region_t region);
bool wf24_perf_frame_begin(wf24_perf_probe_t *probe);
bool wf24_perf_frame_end(wf24_perf_probe_t *probe);
void wf24_perf_add_rendered_samples(wf24_perf_probe_t *probe, uint32_t samples);
void wf24_perf_add_scsp_copy(wf24_perf_probe_t *probe, uint32_t bytes);
void wf24_perf_add_cdfs_read(wf24_perf_probe_t *probe, uint32_t bytes);
void wf24_perf_count_refill(wf24_perf_probe_t *probe);
void wf24_perf_count_underrun(wf24_perf_probe_t *probe);
void wf24_perf_count_timer_a(wf24_perf_probe_t *probe);
void wf24_perf_count_dispatch(wf24_perf_probe_t *probe);

#endif
