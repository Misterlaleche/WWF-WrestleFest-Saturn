#define _GNU_SOURCE
#include "wp02_bgm.h"
#include "ponesound_min.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>

/* Hardware-boundary simulation. The production streamer/codec are linked
 * unchanged. This probe varies only main-loop service cadence; the SCSP
 * continues consuming the original 44100 samples/s in either case. */
static uint8_t *pack, *original;
static unsigned pack_bytes, original_bytes, reads;
static unsigned active_half;
static unsigned service_blocks, max_service_blocks;
static bool playback;
static wf_pcm_ctrl_t controls[32];
static volatile uint16_t *const mon = (volatile uint16_t *)0x25B00408u;
static uint8_t *const ring = (uint8_t *)SCSP_RAM_ADDR(WP02_BGM_RING_OFFSET);

static void *load(const char *path, unsigned *bytes)
{
    FILE *f = fopen(path, "rb"); assert(f);
    assert(!fseek(f, 0, SEEK_END)); *bytes = (unsigned)ftell(f); rewind(f);
    uint8_t *data = calloc(1, (*bytes + 2047u) & ~2047u); assert(data);
    assert(fread(data, 1, *bytes, f) == *bytes); fclose(f); return data;
}

int cd_block_sectors_read(uint32_t fad, void *dst, size_t bytes)
{
    assert(fad >= 100u && bytes % 2048u == 0 && bytes <= 32768u);
    unsigned offset = (fad - 100u) * 2048u;
    assert(offset < pack_bytes && offset + bytes <= ((pack_bytes + 2047u) & ~2047u));
    memcpy(dst, pack + offset, bytes); ++reads; return 0;
}

void wf_ponesound_copy_to_scsp(const void *src, int bytes, int offset)
{
    assert(offset >= WP02_BGM_RING_OFFSET);
    assert(offset + bytes <= WP02_BGM_RING_OFFSET + WP02_BGM_RING_BYTES + 2);
    if (playback && offset < WP02_BGM_RING_OFFSET + WP02_BGM_RING_BYTES)
        assert((unsigned)(offset - WP02_BGM_RING_OFFSET) / WP02_BGM_HALF_BYTES != active_half);
    if (playback && bytes == 2048) ++service_blocks;
    memcpy((void *)SCSP_RAM_ADDR((unsigned)offset), src, (size_t)bytes);
}

void wf_ponesound_pcm_play(short voice, char type, char volume)
{
    assert(voice == 3 && type == 1 && volume == 7);
    controls[voice].loopType = type; controls[voice].volume = (uint8_t)volume;
    controls[voice].sh2_permit = 1;
}

void wf_ponesound_pcm_cease(short voice)
{
    assert(voice == 3); controls[voice].sh2_permit = 0; playback = false;
}

int main(int argc, char **argv)
{
    assert(argc == 4 || argc == 5);
    unsigned cadence = (unsigned)atoi(argv[3]);
    assert(cadence >= 1 && cadence <= 5);
    unsigned rate_millihz = cadence == 4 ? 45000u : cadence == 5 ? 50000u : 59826u;
    unsigned allowed_blocks = argc == 5 ? (unsigned)atoi(argv[4]) : 32;
    pack = load(argv[1], &pack_bytes); original = load(argv[2], &original_bytes);
    assert(mmap((void *)0x00200000u, 0x100000, PROT_READ|PROT_WRITE,
                MAP_PRIVATE|MAP_ANONYMOUS|MAP_FIXED, -1, 0) == (void *)0x00200000u);
    assert(mmap((void *)0x25A00000u, 0x110000, PROT_READ|PROT_WRITE,
                MAP_PRIVATE|MAP_ANONYMOUS|MAP_FIXED, -1, 0) == (void *)0x25A00000u);
    wf_sys_com_t *com = (wf_sys_com_t *)SCSP_RAM_ADDR(0x408 + DRV_SYS_END);
    com->pcmCtrl = controls; controls[3].icsr_target = -1;
    assert(wp02_bgm_load(100, pack_bytes)); unsigned preload_reads = reads;
    assert(wp02_bgm_start()); controls[3].icsr_target = 2;
    assert(wp02_bgm_service(1000)); *mon = 0;
    assert(wp02_bgm_service(1100)); playback = true;
    unsigned played = 0, first_mismatch = 0xFFFFFFFFu, refresh = 0, call = 0;
    while (played * 2u < original_bytes) {
        ++call;
        /* Cadence 3 injects one missed NTSC refresh every ninth call. */
        unsigned refreshes_per_service = cadence == 2 || (cadence == 3 && call % 9u == 0) ? 2 : 1;
        refresh += refreshes_per_service;
        uint64_t time_us = (uint64_t)refresh * 1000000000u / rate_millihz;
        unsigned goal = (unsigned)((uint64_t)refresh * 44100000u / rate_millihz);
        if (goal * 2u > original_bytes) goal = original_bytes / 2u;
        for (; played < goal; ++played) {
            if (first_mismatch == 0xFFFFFFFFu &&
                memcmp(ring + (played % 32768u) * 2u, original + played * 2u, 2))
                first_mismatch = played;
        }
        active_half = (played % 32768u) / 16384u;
        *mon = (uint16_t)(((played % 32768u) / 4096u) << 7);
        service_blocks = 0;
        if (!wp02_bgm_service(1100u + time_us)) {
            printf("FAIL cadence=%u refresh=%u time_us=%llu played=%u refills=%u failures=%u first_mismatch=%u\n",
                   cadence, refresh, (unsigned long long)time_us,
                   played, wp02_bgm_refills(), wp02_bgm_failures(), first_mismatch);
            return 1;
        }
        if (service_blocks > max_service_blocks) max_service_blocks = service_blocks;
        assert(service_blocks <= allowed_blocks);
    }
    assert(first_mismatch == 0xFFFFFFFFu && reads == preload_reads);
    wp02_bgm_stop();
    printf("PASS cadence=%u samples_verified=%u refills=%u runtime_disc_reads=%u max_service_blocks=%u\n",
           cadence, played, wp02_bgm_refills(), reads - preload_reads, max_service_blocks);
    free(pack); free(original); return 0;
}
