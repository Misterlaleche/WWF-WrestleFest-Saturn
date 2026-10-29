#define _GNU_SOURCE
#include "wp02_bgm.h"
#include "wp02_bgm_codec.h"
#include "ponesound_min.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>

static uint8_t *pack;
static unsigned pack_bytes, active_half;
static uint32_t disc_fad = 100;
static bool playback;
static wf_pcm_ctrl_t controls[32];
static volatile uint16_t *const mon = (volatile uint16_t *)0x25B00408u;

static uint8_t *load(const char *path, unsigned *bytes)
{
    FILE *f = fopen(path, "rb"); assert(f);
    assert(!fseek(f, 0, SEEK_END)); *bytes = (unsigned)ftell(f); rewind(f);
    uint8_t *data = calloc(1, (*bytes + 2047u) & ~2047u); assert(data);
    assert(fread(data, 1, *bytes, f) == *bytes); fclose(f); return data;
}

int cd_block_sectors_read(uint32_t fad, void *dst, size_t bytes)
{
    assert(fad >= disc_fad && bytes % 2048u == 0 && bytes <= 32768u);
    unsigned offset = (fad - disc_fad) * 2048u;
    assert(offset < pack_bytes && offset + bytes <= ((pack_bytes + 2047u) & ~2047u));
    memcpy(dst, pack + offset, bytes); return 0;
}

void wf_ponesound_copy_to_scsp(const void *src, int bytes, int offset)
{
    assert(offset >= WP02_BGM_RING_OFFSET);
    assert(offset + bytes <= WP02_BGM_RING_OFFSET + WP02_BGM_RING_BYTES + 2);
    if (playback && offset < WP02_BGM_RING_OFFSET + WP02_BGM_RING_BYTES)
        assert((unsigned)(offset - WP02_BGM_RING_OFFSET) / WP02_BGM_HALF_BYTES != active_half);
    memcpy((void *)SCSP_RAM_ADDR((unsigned)offset), src, (size_t)bytes);
}

void wf_ponesound_pcm_play(short voice, char type, char volume)
{
    assert(voice == 3 && type == 1 && volume == 7);
    controls[voice].sh2_permit = 1;
}

void wf_ponesound_pcm_cease(short voice)
{
    assert(voice == 3); controls[voice].sh2_permit = 0; playback = false;
}

static unsigned offset_after_blocks(unsigned count)
{
    unsigned offset = WP02_BGM_HEADER_BYTES;
    for (unsigned i = 0; i < count; ++i) {
        assert(offset + WP02_BGM_BLOCK_HEADER_BYTES <= pack_bytes);
        offset += WP02_BGM_BLOCK_HEADER_BYTES + ((unsigned)pack[offset + 2] << 8) + pack[offset + 3];
    }
    return offset;
}

static void fresh(bool ready)
{
    ++disc_fad;
    controls[3].icsr_target = -1;
    assert(wp02_bgm_load(disc_fad, pack_bytes));
    assert(wp02_bgm_failure_reason() == WP02_BGM_FAILURE_NONE);
    assert(wp02_bgm_start());
    if (ready) {
        controls[3].icsr_target = 6;
        assert(wp02_bgm_service(1000));
        *mon = 0; active_half = 0;
        assert(wp02_bgm_service(2100)); playback = true;
    }
}

static wp02_bgm_failure_snapshot_t failure(wp02_bgm_failure_reason_t reason)
{
    assert(!wp02_bgm_is_running() && wp02_bgm_failures() == 1);
    assert(wp02_bgm_failure_reason() == reason);
    wp02_bgm_failure_snapshot_t saved = *wp02_bgm_failure_snapshot();
    assert(saved.reason == reason);
    /* Repeated service/stop/start attempts must not replace the first record. */
    assert(!wp02_bgm_service(60000000));
    wp02_bgm_stop(); assert(!wp02_bgm_start());
    assert(!memcmp(&saved, wp02_bgm_failure_snapshot(), sizeof(saved)));
    assert(wp02_bgm_failures() == 1);
    wp02_bgm_reset();
    assert(wp02_bgm_failure_reason() == WP02_BGM_FAILURE_NONE);
    const wp02_bgm_failure_snapshot_t *clear = wp02_bgm_failure_snapshot();
    assert(clear->refills == 0 && clear->fill_bytes == 0 && clear->service_gap_us == 0);
    assert(clear->source_samples == 0 && clear->source_offset == 0);
    return saved;
}

int main(int argc, char **argv)
{
    _Static_assert(sizeof(wp02_bgm_failure_snapshot_t) == 24, "compact snapshot");
    assert(argc == 2); pack = load(argv[1], &pack_bytes);
    assert(mmap((void *)0x00200000u, 0x100000, PROT_READ|PROT_WRITE,
                MAP_PRIVATE|MAP_ANONYMOUS|MAP_FIXED, -1, 0) == (void *)0x00200000u);
    assert(mmap((void *)0x25A00000u, 0x110000, PROT_READ|PROT_WRITE,
                MAP_PRIVATE|MAP_ANONYMOUS|MAP_FIXED, -1, 0) == (void *)0x25A00000u);
    wf_sys_com_t *com = (wf_sys_com_t *)SCSP_RAM_ADDR(0x408 + DRV_SYS_END);
    com->pcmCtrl = controls;

    assert(!wp02_bgm_load(0, pack_bytes));
    wp02_bgm_failure_snapshot_t s = failure(WP02_BGM_FAILURE_LOAD);
    assert(s.target == -1 && s.raw_monitor == UINT16_MAX);

    fresh(false);
    assert(wp02_bgm_service(1000));
    assert(!wp02_bgm_service(101000));
    s = failure(WP02_BGM_FAILURE_SLOT_TIMEOUT);
    assert(s.target == -1 && s.service_gap_us == 100000);

    fresh(true);
    assert(!wp02_bgm_service(402100));
    s = failure(WP02_BGM_FAILURE_SERVICE_GAP);
    assert(s.service_gap_us == 400000 && s.raw_monitor == UINT16_MAX);

    fresh(true); controls[3].icsr_target = -1;
    assert(!wp02_bgm_service(3100));
    s = failure(WP02_BGM_FAILURE_SLOT_LOST);
    assert(s.target == -1 && s.service_gap_us == 1000);

    fresh(true); controls[3].icsr_target = 32;
    assert(!wp02_bgm_service(3100));
    s = failure(WP02_BGM_FAILURE_SLOT_INVALID);
    assert(s.target == 32);

    fresh(true); controls[3].icsr_target = 7;
    assert(!wp02_bgm_service(3100));
    s = failure(WP02_BGM_FAILURE_SLOT_CHANGED);
    assert(s.target == 7);

    fresh(true); *mon = (8u << 7);
    assert(!wp02_bgm_service(3100));
    s = failure(WP02_BGM_FAILURE_MONITOR_RANGE);
    assert(s.raw_monitor == (8u << 7) && s.target == 6);

    /* Invalid block mode after the 32 primed blocks proves that a runtime
     * decoder error is classified without writing to the playing half. */
    fresh(true);
    unsigned next_offset = offset_after_blocks(32);
    assert(next_offset + 4u < WP02_BGM_LOW_BYTES);
    ((uint8_t *)0x00200000u)[next_offset + 4u] = 255;
    *mon = (4u << 7); active_half = 1;
    assert(!wp02_bgm_service(18700));
    s = failure(WP02_BGM_FAILURE_DECODE);
    assert(s.source_offset == next_offset && s.source_samples == 32768);
    assert(s.raw_monitor == (4u << 7) && s.refills == 0 && s.fill_bytes == 0);

    /* A sustained 10 Hz service rate is outside the two-block budget. The
     * existing fail-closed behavior must retain the unfinished half record. */
    fresh(true);
    for (unsigned step = 1; step <= 8; ++step) {
        unsigned sample = step * 4410u;
        active_half = (sample % 32768u) / 16384u;
        *mon = (uint16_t)(((sample % 32768u) / 4096u) << 7);
        bool ok = wp02_bgm_service(2100u + (uint64_t)step * 100000u);
        assert(ok == (step < 8));
    }
    s = failure(WP02_BGM_FAILURE_REFILL_LATE);
    assert(s.raw_monitor == 0 && s.target == 6 && s.service_gap_us == 100000);
    assert(s.fill_bytes == 14336 && s.refills == 7);
    assert(s.source_samples == 39936 && s.source_offset == offset_after_blocks(39));

    /* Continue through the padded final half. Exhaustion must remain distinct
     * from a late refill and report the end of the finite source. */
    fresh(true);
    bool stopped = false;
    for (unsigned frame = 1; frame <= 1800; ++frame) {
        unsigned sample = (unsigned)((uint64_t)frame * 44100000u / 59826u);
        active_half = (sample % 32768u) / 16384u;
        *mon = (uint16_t)(((sample % 32768u) / 4096u) << 7);
        if (!wp02_bgm_service(2100u + (uint64_t)frame * 1000000000u / 59826u)) {
            stopped = true; break;
        }
    }
    assert(stopped);
    s = failure(WP02_BGM_FAILURE_SOURCE_EXHAUSTED);
    assert(s.source_samples == 1278900 && s.source_offset == pack_bytes);
    assert(s.fill_bytes == 0);

    free(pack);
    puts("PASS compact first-failure snapshot: 10 reasons, original stop behavior, source position, raw monitor, late fill, immutability, reset");
    return 0;
}
