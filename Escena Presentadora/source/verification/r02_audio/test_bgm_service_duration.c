#define _GNU_SOURCE
#include "wp02_bgm.h"
#include "ponesound_min.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>

/* Focused counterexample: the actual streamer runs unchanged while the
 * hardware boundary advances playback by a selected decode duration before
 * each full-block copy. This is a timing model, not a hardware measurement. */
static uint8_t *pack;
static unsigned pack_bytes, unsafe_writes;
static bool playback;
static uint64_t playback_us;
static wf_pcm_ctrl_t controls[32];
static volatile uint16_t *const mon = (volatile uint16_t *)0x25B00408u;

static void set_playback_time(uint64_t time_us)
{
    playback_us = time_us;
    unsigned sample = (unsigned)(time_us * 44100u / 1000000u) % 32768u;
    *mon = (uint16_t)((sample / 4096u) << 7);
}

int cd_block_sectors_read(uint32_t fad, void *dst, size_t bytes)
{
    assert(fad >= 100 && bytes % 2048u == 0 && bytes <= 32768u);
    unsigned offset = (fad - 100u) * 2048u;
    assert(offset < pack_bytes && offset + bytes <= ((pack_bytes + 2047u) & ~2047u));
    memcpy(dst, pack + offset, bytes); return 0;
}

void wf_ponesound_copy_to_scsp(const void *src, int bytes, int offset)
{
    assert(offset >= WP02_BGM_RING_OFFSET);
    assert(offset + bytes <= WP02_BGM_RING_OFFSET + WP02_BGM_RING_BYTES + 2);
    if (playback && bytes == 2048) {
        set_playback_time(playback_us + 4000u);
        unsigned active = ((*mon >> 7) & 7u) >> 2;
        unsigned destination = (unsigned)(offset - WP02_BGM_RING_OFFSET) / WP02_BGM_HALF_BYTES;
        if (active == destination) ++unsafe_writes;
    }
    memcpy((void *)SCSP_RAM_ADDR((unsigned)offset), src, (size_t)bytes);
}

void wf_ponesound_pcm_play(short voice, char type, char volume)
{ assert(voice == 3 && type == 1 && volume == 7); }
void wf_ponesound_pcm_cease(short voice)
{ assert(voice == 3); playback = false; }

int main(int argc, char **argv)
{
    assert(argc == 3 || argc == 4);
    unsigned expected_unsafe = (unsigned)atoi(argv[2]);
    unsigned second_call_us = argc == 4 ? (unsigned)atoi(argv[3]) : 738000u;
    FILE *f = fopen(argv[1], "rb"); assert(f);
    assert(!fseek(f, 0, SEEK_END)); pack_bytes = (unsigned)ftell(f); rewind(f);
    pack = calloc(1, (pack_bytes + 2047u) & ~2047u); assert(pack);
    assert(fread(pack, 1, pack_bytes, f) == pack_bytes); fclose(f);
    assert(mmap((void *)0x00200000u, 0x100000, PROT_READ|PROT_WRITE,
                MAP_PRIVATE|MAP_ANONYMOUS|MAP_FIXED, -1, 0) == (void *)0x00200000u);
    assert(mmap((void *)0x25A00000u, 0x110000, PROT_READ|PROT_WRITE,
                MAP_PRIVATE|MAP_ANONYMOUS|MAP_FIXED, -1, 0) == (void *)0x25A00000u);
    wf_sys_com_t *com = (wf_sys_com_t *)SCSP_RAM_ADDR(0x408 + DRV_SYS_END);
    com->pcmCtrl = controls; controls[3].icsr_target = -1;
    assert(wp02_bgm_load(100, pack_bytes));
    assert(wp02_bgm_start()); controls[3].icsr_target = 6;
    assert(wp02_bgm_service(1000)); *mon = 0;
    assert(wp02_bgm_service(1100)); playback = true;

    /* Detect the first half switch, then experience one large but still
     * permitted gap. Both calls begin before the first full-ring wrap. */
    set_playback_time(200000);
    assert(wp02_bgm_service(201100));
    set_playback_time(400000);
    assert(wp02_bgm_service(401100));
    assert(unsafe_writes == 0);
    set_playback_time(second_call_us);
    assert(wp02_bgm_service(second_call_us + 1100u));
    bool matched = unsafe_writes == expected_unsafe;
    printf("%s timing-model expectation: block_us=4000 second_call_us=%u end_us=%llu refills=%u active_half_writes=%u expected=%u\n",
           matched ? "PASS" : "FAIL", second_call_us,
           (unsigned long long)playback_us, wp02_bgm_refills(), unsafe_writes, expected_unsafe);
    wp02_bgm_stop(); free(pack); return matched ? 0 : 1;
}
