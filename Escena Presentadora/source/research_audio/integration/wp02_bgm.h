#ifndef WP02_BGM_H
#define WP02_BGM_H
#include <stdbool.h>
#include <stdint.h>

enum {
    WP02_BGM_RING_OFFSET = 0x6C800,
    WP02_BGM_RING_BYTES = 0x10000,
    WP02_BGM_HALF_BYTES = 0x8000,
    WP02_BGM_LOGICAL_VOICE = 3,
    WP02_BGM_LOW_BYTES = 0x100000,
    WP02_BGM_HIGH_BYTES = 704 * 1024
};

typedef enum {
    WP02_BGM_FAILURE_NONE = 0,
    WP02_BGM_FAILURE_LOAD = 1,
    WP02_BGM_FAILURE_DECODE = 2,
    WP02_BGM_FAILURE_SERVICE_GAP = 3,
    WP02_BGM_FAILURE_SLOT_TIMEOUT = 4,
    WP02_BGM_FAILURE_SLOT_LOST = 5,
    WP02_BGM_FAILURE_SLOT_INVALID = 6,
    WP02_BGM_FAILURE_SLOT_CHANGED = 7,
    WP02_BGM_FAILURE_MONITOR_RANGE = 8,
    WP02_BGM_FAILURE_REFILL_LATE = 9,
    WP02_BGM_FAILURE_SOURCE_EXHAUSTED = 10
} wp02_bgm_failure_reason_t;

/* Frozen before the first failure stops playback; reset by wp02_bgm_reset().
 * raw_monitor is UINT16_MAX when this service did not read CA. target is -1
 * if unavailable. fill_bytes is zero when there is no unfinished refill. */
typedef struct {
    uint32_t service_gap_us;
    uint32_t source_samples;
    uint32_t source_offset;
    uint32_t refills;
    uint16_t raw_monitor;
    uint16_t fill_bytes;
    int16_t target;
    uint16_t reason;
} wp02_bgm_failure_snapshot_t;

bool wp02_bgm_load(uint32_t fad, uint32_t bytes);
void wp02_bgm_reset(void);
bool wp02_bgm_start(void);
bool wp02_bgm_service(uint64_t now_us);
void wp02_bgm_stop(void);
unsigned wp02_bgm_refills(void);
unsigned wp02_bgm_failures(void);
bool wp02_bgm_is_running(void);
wp02_bgm_failure_reason_t wp02_bgm_failure_reason(void);
const wp02_bgm_failure_snapshot_t *wp02_bgm_failure_snapshot(void);
#endif
