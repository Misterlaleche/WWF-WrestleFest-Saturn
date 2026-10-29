#include "wp02_bgm.h"
#include "wp02_bgm_codec.h"
#include "ponesound_min.h"
#include <yaul.h>
#include <string.h>

/* Compressed source is resident for the full scene. LWRAM is reserved by the
 * caller; HWRAM holds only the overflow. Reloads reuse these bytes. */
static uint8_t high_source[WP02_BGM_HIGH_BYTES] __aligned(32);
static uint8_t encoded[WP02_BGM_BLOCK_MAX_BYTES] __aligned(32);
static uint8_t decoded[WP02_BGM_BLOCK_SAMPLES * 2u] __aligned(32);
static uint8_t * const low_source = (uint8_t *)0x00200000u;
static wf_sys_com_t * const com = (wf_sys_com_t *)SCSP_RAM_ADDR(0x408 + DRV_SYS_END);
static volatile uint16_t * const monitor = (volatile uint16_t *)0x25B00408u;
static wp02_bgm_header_t header;
static uint32_t cached_fad, cached_bytes, source_offset, source_samples;
static unsigned block_count, refill_count, fill_half, fill_bytes;
static unsigned failure_count;
static unsigned active_half, eof_half;
static int monitor_target = -1;
static uint64_t monitor_selected_us, last_service_us;
static uint64_t first_service_us;
static bool armed, running, failed, filling, saw_eof, monitor_ready;
static wp02_bgm_failure_snapshot_t last_failure = {
    .raw_monitor = UINT16_MAX, .target = -1
};
_Static_assert(sizeof(last_failure) == 24u, "compact BGM failure record");

static unsigned get16(const uint8_t *p) { return ((unsigned)p[0] << 8) | p[1]; }
static bool source_copy(uint32_t offset, void *dst, unsigned bytes)
{
    if (offset > cached_bytes || bytes > cached_bytes - offset) return false;
    uint8_t *out = dst;
    if (offset < WP02_BGM_LOW_BYTES) {
        unsigned n = WP02_BGM_LOW_BYTES - offset;
        if (n > bytes) n = bytes;
        memcpy(out, low_source + offset, n);
        out += n; offset += n; bytes -= n;
    }
    if (bytes) memcpy(out, high_source + offset - WP02_BGM_LOW_BYTES, bytes);
    return true;
}

static void copy_ring(unsigned offset, const void *src, unsigned bytes)
{
    wf_ponesound_copy_to_scsp(src, (int)bytes, WP02_BGM_RING_OFFSET + (int)offset);
    /* LEA is the endpoint offset. Keep its interpolation guard valid too. */
    if (offset == 0u && bytes >= 2u)
        wf_ponesound_copy_to_scsp(src, 2, WP02_BGM_RING_OFFSET + WP02_BGM_RING_BYTES);
}

void wp02_bgm_stop(void)
{
    if (running) wf_ponesound_pcm_cease(WP02_BGM_LOGICAL_VOICE);
    running = false; armed = false; monitor_ready = false; monitor_target = -1;
}

void wp02_bgm_reset(void)
{
    wp02_bgm_stop();
    failed = false; filling = false; saw_eof = false; failure_count = 0;
    memset(&last_failure, 0, sizeof(last_failure));
    last_failure.raw_monitor = UINT16_MAX; last_failure.target = -1;
    /* cached_fad/bytes and resident source survive a sound-driver reload. */
}

static bool fail(wp02_bgm_failure_reason_t reason, uint64_t gap_us,
                 int target, uint16_t raw_monitor)
{
    if (!failed) {
        last_failure.service_gap_us = gap_us > UINT32_MAX ? UINT32_MAX : (uint32_t)gap_us;
        last_failure.source_samples = reason == WP02_BGM_FAILURE_LOAD ? 0u : source_samples;
        last_failure.source_offset = reason == WP02_BGM_FAILURE_LOAD ? 0u : source_offset;
        last_failure.refills = reason == WP02_BGM_FAILURE_LOAD ? 0u : refill_count;
        last_failure.raw_monitor = raw_monitor;
        last_failure.fill_bytes = filling ? (uint16_t)fill_bytes : 0u;
        last_failure.target = (int16_t)target;
        last_failure.reason = (uint16_t)reason;
    }
    wp02_bgm_stop(); failed = true; ++failure_count; return false;
}

static bool fill_block(unsigned offset, unsigned half)
{
    unsigned samples = 0, consumed = 0;
    memset(decoded, 0, sizeof(decoded));
    if (source_samples < header.samples) {
        if (!source_copy(source_offset, encoded, WP02_BGM_BLOCK_HEADER_BYTES)) return false;
        unsigned bytes = WP02_BGM_BLOCK_HEADER_BYTES + get16(encoded + 2);
        if (bytes > sizeof(encoded) || !source_copy(source_offset, encoded, bytes) ||
            !wp02_bgm_block_decode(encoded, bytes, decoded, sizeof(decoded), &consumed, &samples) ||
            consumed != bytes || samples > header.samples - source_samples)
            return false;
        source_offset += consumed; source_samples += samples; ++block_count;
        if (source_samples == header.samples) {
            if (source_offset != cached_bytes || block_count != header.blocks) return false;
            saw_eof = true; eof_half = half;
        } else if (samples != WP02_BGM_BLOCK_SAMPLES) return false;
    }
    /* Only the finite final source half is padded. Leaving it without the
     * scene stopping is an explicit exhaustion error, never a content loop. */
    copy_ring(offset, decoded, sizeof(decoded));
    return true;
}

bool wp02_bgm_load(uint32_t fad, uint32_t bytes)
{
    wp02_bgm_reset();
    if (!fad || bytes < WP02_BGM_HEADER_BYTES ||
        bytes > WP02_BGM_LOW_BYTES + WP02_BGM_HIGH_BYTES)
        return fail(WP02_BGM_FAILURE_LOAD, 0, -1, UINT16_MAX);
    if (fad != cached_fad || bytes != cached_bytes) {
        cached_fad = 0; cached_bytes = 0;
        for (unsigned off = 0; off < bytes;) {
            unsigned count = bytes - off;
            if (count > 32768u) count = 32768u;
            uint8_t *dst = off < WP02_BGM_LOW_BYTES ? low_source + off :
                           high_source + off - WP02_BGM_LOW_BYTES;
            /* Both reserved regions and the transfer chunk are sector aligned.
             * Final-sector padding remains inside the reserved source capacity. */
            unsigned transfer = (count + 2047u) & ~2047u;
            if (cd_block_sectors_read(fad + off / 2048u, dst, transfer))
                return fail(WP02_BGM_FAILURE_LOAD, 0, -1, UINT16_MAX);
            off += count;
        }
        cached_fad = fad; cached_bytes = bytes;
    }
    if (!source_copy(0, encoded, WP02_BGM_HEADER_BYTES) ||
        !wp02_bgm_header_read(encoded, bytes, &header) || header.samples < 44100u * 29u)
        return fail(WP02_BGM_FAILURE_LOAD, 0, -1, UINT16_MAX);
    source_offset = WP02_BGM_HEADER_BYTES; source_samples = 0;
    block_count = refill_count = 0; active_half = 0; eof_half = 2;
    for (unsigned off = 0; off < WP02_BGM_RING_BYTES; off += sizeof(decoded))
        if (!fill_block(off, off / WP02_BGM_HALF_BYTES))
            return fail(WP02_BGM_FAILURE_DECODE, 0, -1, UINT16_MAX);
    volatile wf_pcm_ctrl_t *c = &com->pcmCtrl[WP02_BGM_LOGICAL_VOICE];
    c->hiAddrBits = WP02_BGM_RING_OFFSET >> 16;
    c->loAddrBits = WP02_BGM_RING_OFFSET & 65535u;
    c->LSA = 0;
    /* Pinned driver writes LEA=playsize-1. SCSP forward-loop endpoint32768
     * gives exactly32768 samples, with a separate endpoint guard word. */
    c->playsize = 32769;
    c->pitchword = 0; /* Native44.1kHz; avoids signed-short rate helper. */
    c->pan = 0; c->bytes_per_blank = 1470; c->bitDepth = 0;
    c->loopType = 1; c->volume = 0; c->sh2_permit = 0;
    armed = true;
    return true;
}

bool wp02_bgm_start(void)
{
    if (failed) return false;
    if (!armed) return true;
    armed = false; running = true; filling = false;
    monitor_target = -1; monitor_ready = false; last_service_us = 0;
    first_service_us = UINT64_MAX;
    wf_ponesound_pcm_play(WP02_BGM_LOGICAL_VOICE, 1, 7);
    return true;
}

bool wp02_bgm_service(uint64_t now_us)
{
    if (failed) return false;
    if (!running) return true;
    if (first_service_us == UINT64_MAX) first_service_us = now_us;
    uint64_t gap_us = last_service_us ? now_us - last_service_us : 0u;
    if (gap_us >= 371519u)
        return fail(WP02_BGM_FAILURE_SERVICE_GAP, gap_us, -1, UINT16_MAX);
    /* One block covers 1024/44100 seconds. A delayed call may decode a
     * second block; normal NTSC calls retain the one-block work budget. */
    unsigned budget = gap_us >= 23220u ? 2u : 1u;
    last_service_us = now_us;
    int target = com->pcmCtrl[WP02_BGM_LOGICAL_VOICE].icsr_target;
    if (target < 0) {
        if (monitor_target >= 0)
            return fail(WP02_BGM_FAILURE_SLOT_LOST, gap_us, target, UINT16_MAX);
        if (now_us - first_service_us >= 100000u)
            return fail(WP02_BGM_FAILURE_SLOT_TIMEOUT, gap_us, target, UINT16_MAX);
        return true; /* Waiting briefly for first M68K command service. */
    }
    if (target > 31)
        return fail(WP02_BGM_FAILURE_SLOT_INVALID, gap_us, target, UINT16_MAX);
    if (target != monitor_target) {
        if (monitor_ready) /* A playing loop must retain its slot. */
            return fail(WP02_BGM_FAILURE_SLOT_CHANGED, gap_us, target, UINT16_MAX);
        *monitor = (uint16_t)((unsigned)target << 11);
        monitor_target = target; monitor_selected_us = now_us;
        return true;
    }
    if (!monitor_ready) {
        if (now_us - monitor_selected_us < 25u) return true;
        monitor_ready = true;
    }
    /* Pinned M68K driver never touches MSLC. This helper is its sole owner.
     * The latched CA is a4096-sample bucket, not an exact sample pointer. */
    uint16_t raw_monitor = *monitor;
    unsigned ca = (raw_monitor >> 7) & 15u;
    if (ca > 7u)
        return fail(WP02_BGM_FAILURE_MONITOR_RANGE, gap_us, target, raw_monitor);
    /* The last 4096-sample bucket has no positive time-to-boundary bound.
     * Keep R01's single-block policy there instead of adding a second copy. */
    if ((ca & 3u) == 3u) budget = 1u;
    unsigned next_active = ca >> 2;
    if (next_active != active_half) {
        if (filling)
            return fail(WP02_BGM_FAILURE_REFILL_LATE, gap_us, target, raw_monitor);
        if (saw_eof && active_half == eof_half)
            return fail(WP02_BGM_FAILURE_SOURCE_EXHAUSTED, gap_us, target, raw_monitor);
        fill_half = active_half; fill_bytes = 0; filling = true;
        active_half = next_active;
    }
    for (; filling && budget; --budget) {
        if (!fill_block(fill_half * WP02_BGM_HALF_BYTES + fill_bytes, fill_half))
            return fail(WP02_BGM_FAILURE_DECODE, gap_us, target, raw_monitor);
        fill_bytes += sizeof(decoded); ++refill_count;
        if (fill_bytes == WP02_BGM_HALF_BYTES) filling = false;
    }
    return true;
}

unsigned wp02_bgm_refills(void) { return refill_count; }
unsigned wp02_bgm_failures(void) { return failure_count; }
bool wp02_bgm_is_running(void) { return running; }
wp02_bgm_failure_reason_t wp02_bgm_failure_reason(void)
{ return (wp02_bgm_failure_reason_t)last_failure.reason; }
const wp02_bgm_failure_snapshot_t *wp02_bgm_failure_snapshot(void)
{ return &last_failure; }
