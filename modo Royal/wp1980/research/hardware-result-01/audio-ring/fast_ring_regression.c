/* Exercise the production WF_SFX_FAST_68000 fill/work/advance path on a host.
 * The only substitutions are a relocated sound-RAM address and the 68000
 * assembly decoder. The decoder keeps its real contiguous-write contract;
 * it emits deterministic samples so ordering and neighboring writes are visible.
 * The test catches a missing ring-tail bound, skipped samples, wrong wrap
 * destination, stale interpolation guard, and writes into another voice.
 */
#define _GNU_SOURCE
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>

/* Preserve the actual 68000 decoder ABI asserted by the production source. */
#pragma pack(push, 2)
#include "wf_scsp_sfx.h"
#pragma pack(pop)

#define TEST_RAM_BASE 0x18000000u
#define TEST_RAM_BYTES 0x0000c000u
/* Header enum is already declared. Relocate only MMIO pointers, not ring math. */
#define WF_SFX_PCM_BASE (TEST_RAM_BASE + 0x3000u)
#define WF_SFX_FAST_68000 1
#ifndef WF_SFX_SOURCE
#error Pass the unmodified production source path as WF_SFX_SOURCE
#endif
#include WF_SFX_SOURCE

static uint16_t before_ram[TEST_RAM_BYTES / 2u];
static uint16_t expected_ram[TEST_RAM_BYTES / 2u];
static uint8_t dummy_pack[32];
static unsigned failures, cases_run, decoder_calls;

static uint16_t sample(unsigned n)
{
    /* Never equal to the 0xaaaa sentinel; unique across the witness window. */
    return (uint16_t)(0x1000u | (n & 0x0fffu));
}

void wf_sfx_decode_block(wf_sfx_decoder_t *d, const uint8_t *pack,
                         volatile uint16_t *out, unsigned count)
{
    uintptr_t address = (uintptr_t)out;
    (void)pack;
    if (count > 256u || address < TEST_RAM_BASE ||
        address + count * 2u > TEST_RAM_BASE + TEST_RAM_BYTES) {
        fprintf(stderr, "Invalid test decoder destination/count\n");
        exit(2);
    }
    ++decoder_calls;
    /* Deliberately do not wrap or reject a ring crossing: neither does asm. */
    for (unsigned i = 0; i < count; ++i) {
        if (d->remaining) {
            out[i] = sample(d->offset++);
            --d->remaining;
        } else {
            out[i] = 0;
        }
    }
}

static bool write16_sink(uint32_t address, uint16_t value, void *user)
{
    (void)address; (void)value; (void)user;
    return true;
}

static bool pcm_control_sink(unsigned voice, bool start, unsigned volume,
                             void *user)
{
    (void)voice; (void)start; (void)volume; (void)user;
    return true;
}

static void expect(const char *test, bool ok, const char *description)
{
    if (!ok) {
        ++failures;
        printf("FAIL %s: %s\n", test, description);
    }
}

static unsigned base_word(unsigned voice, unsigned stride)
{
    return (0x3000u + voice * stride) / 2u;
}

static void prepare(wf_scsp_sfx_t *s, unsigned voice, unsigned stride,
                    uint32_t played, uint32_t write_at, uint32_t write_end)
{
    volatile uint16_t *ram = (volatile uint16_t *)(uintptr_t)TEST_RAM_BASE;
    for (unsigned i = 0; i < TEST_RAM_BYTES / 2u; ++i)
        ram[i] = 0xaaaau;
    memcpy(before_ram, (const void *)(uintptr_t)TEST_RAM_BASE, sizeof before_ram);
    memcpy(expected_ram, before_ram, sizeof expected_ram);
    wf_scsp_sfx_init(s, write16_sink, NULL);
    if (stride == 8194u &&
        !wf_scsp_sfx_configure_pcm(s, stride, 1, pcm_control_sink))
        abort();
    s->pack = dummy_pack;
    s->bound = 1;
    s->frames = 100000u;
    s->pcm[voice].playing = 1;
    s->pcm[voice].played = played;
    s->pcm[voice].last_frame = s->frames;
    s->pcm[voice].write_at = write_at;
    s->pcm[voice].write_end = write_end;
    s->pcm[voice].total_frames = 60000u;
    s->pcm[voice].decoder.offset = write_at;
    s->pcm[voice].decoder.remaining = 60000u - write_at;
    decoder_calls = 0;
}

static void expected_output(unsigned voice, unsigned stride,
                            unsigned first, unsigned end)
{
    unsigned base = base_word(voice, stride);
    for (unsigned n = first; n < end; ++n)
        expected_ram[base + n % 4096u] = sample(n);
    if (stride == 8194u)
        expected_ram[base + 4096u] = expected_ram[base];
}

static void check_ram(const char *test, unsigned voice, unsigned stride)
{
    const volatile uint16_t *ram =
        (const volatile uint16_t *)(uintptr_t)TEST_RAM_BASE;
    unsigned base = base_word(voice, stride);
    unsigned stale = 0, outside = 0, next_voice = 0;
    unsigned owned_end = base + stride / 2u;
    for (unsigned i = 0; i < TEST_RAM_BYTES / 2u; ++i) {
        if (ram[i] == expected_ram[i])
            continue;
        if (i >= base && i < base + 4096u)
            ++stale;
        else if (i < base || i >= owned_end) {
            ++outside;
            if (voice < 3u && i >= owned_end &&
                i < owned_end + stride / 2u)
                ++next_voice;
        }
    }
    printf("OBS %s own_ring_wrong=%u outside_voice_changed=%u "
           "next_voice_changed=%u guard_match=%u decoder_calls=%u\n",
           test, stale, outside, next_voice,
           stride != 8194u || ram[base + 4096u] == ram[base], decoder_calls);
    expect(test, stale == 0, "decoded samples must occupy the correct ring indices in order");
    expect(test, outside == 0, "other voices and surrounding memory must remain unchanged");
    if (stride == 8194u)
        expect(test, ram[base + 4096u] == ram[base],
               "interpolation guard must mirror sample zero after wrap");
}

static void boundary_witness(unsigned voice, unsigned stride)
{
    wf_scsp_sfx_t s;
    char name[80];
    snprintf(name, sizeof name, "boundary_voice%u_stride%u", voice, stride);
    ++cases_run;
    prepare(&s, voice, stride, 4128u, 8160u, 8160u);
    wf_scsp_sfx_advance(&s, 353u);
    expect(name, s.pcm[voice].played == 4192u &&
           s.pcm[voice].write_end == 8224u,
           "the actual advance path must reach the hand-derived witness");
    expect(name, s.underruns == 0 && s.pcm[voice].playing,
           "witness must remain valid without triggering the stall policy");
    /* A bounded worker may defer the wrapped half until its next call. */
    for (unsigned call = 0; call < 3u && s.pcm[voice].write_at < 8224u; ++call)
        wf_scsp_sfx_work(&s);
    expected_output(voice, stride, 8160u, 8224u);
    check_ram(name, voice, stride);
    expect(name, s.pcm[voice].write_at == 8224u &&
           s.pcm[voice].decoder.offset == 8224u &&
           s.pcm[voice].decoder.remaining == 51776u,
           "bounded work must preserve all 64 samples exactly once");
    expect(name, !s.fault && !s.underruns, "valid wrap must not fault or underrun");
}

static void bounded_case(const char *name, unsigned at, unsigned end,
                         unsigned expected_after, unsigned initial_fill)
{
    wf_scsp_sfx_t s;
    ++cases_run;
    prepare(&s, 2u, 8194u, 4000u, at, end);
    s.pcm[2].initial_fill = (uint8_t)initial_fill;
    wf_scsp_sfx_work(&s);
    expected_output(2u, 8194u, at, expected_after);
    check_ram(name, 2u, 8194u);
    expect(name, s.pcm[2].write_at == expected_after &&
           s.pcm[2].decoder.offset == expected_after,
           "work must honor both available samples and its 64-sample budget");
    expect(name, s.pcm[2].initial_fill ==
           (initial_fill && expected_after != end),
           "initial-fill completion must still follow write_end");
}

int main(void)
{
    void *memory = mmap((void *)(uintptr_t)TEST_RAM_BASE, TEST_RAM_BYTES,
                        PROT_READ | PROT_WRITE,
                        MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED_NOREPLACE,
                        -1, 0);
    if (memory == MAP_FAILED) {
        perror("mmap test sound RAM");
        return 2;
    }
    for (unsigned stride = 8192u; stride <= 8194u; stride += 2u)
        for (unsigned voice = 0; voice < 4u; ++voice)
            boundary_witness(voice, stride);
    bounded_case("ordinary_fill", 128u, 320u, 192u, 0u);
    bounded_case("available_tail_only", 8160u, 8176u, 8176u, 0u);
    bounded_case("exact_ring_end", 8128u, 8192u, 8192u, 0u);
    bounded_case("initial_fill_completion", 4032u, 4096u, 4096u, 1u);
    bounded_case("empty_work", 8192u, 8192u, 8192u, 0u);
    munmap(memory, TEST_RAM_BYTES);
    printf("RESULT cases=%u failures=%u\n", cases_run, failures);
    return failures ? 1 : 0;
}
