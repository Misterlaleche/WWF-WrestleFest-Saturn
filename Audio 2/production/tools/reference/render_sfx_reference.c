/* HOST ONLY: independent preserved A15 command model + audited ROM PCM.
 * 8 kHz OKI zero-order hold, Timer A = 3579545/(64*253) Hz, 44.1 kHz output.
 * Every event is ordered by exact rational time. Command is accepted at t=0.
 * The 21627/32768 route gain matches the documented R04 OKI mix convention;
 * it is a software reference calibration, not an original-ROM constant.
 * No native-driver code is linked; these are model renders, not captures. */
#include "wf_audio_sfx.h"
#include "sfx_fixture.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct { unsigned offset, frames; } asset_t;
typedef struct {
    asset_t asset;
    unsigned position, volume, bank, sample;
    int current, playing, suppress;
} physical_t;
static asset_t assets[2][74];
static physical_t voices[3];
static unsigned char *pcm;
static unsigned starts, suppressed, stops, natural_ends;
static FILE *events;
static uint64_t time_num, time_den;

static void event(const char *kind, unsigned voice, unsigned bank, unsigned sample, unsigned volume)
{
    fprintf(events, "%llu,%llu,%s,%u,%u,%u,%u\n", (unsigned long long)time_num,
            (unsigned long long)time_den, kind, voice, bank, sample, volume);
}

static void stop(unsigned voice, void *unused)
{
    (void)unused;
    assert(voice < 3);
    event("stop", voice, voices[voice].bank, voices[voice].sample, voices[voice].volume);
    voices[voice] = (physical_t){0};
    stops++;
}

static void play(unsigned voice, unsigned bank, unsigned sample, unsigned volume,
                 bool suppress, void *unused)
{
    (void)unused;
    assert(voice < 3 && bank < 2 && sample < 74 && assets[bank][sample].frames);
    physical_t *v = &voices[voice];
    if (v->playing && v->suppress) {
        v->suppress = suppress;
        event("suppressed_start", voice, bank, sample, volume);
        suppressed++;
        return;
    }
    *v = (physical_t){assets[bank][sample], 0, volume, bank, sample, 0, 1, suppress};
    event("start", voice, bank, sample, volume);
    starts++;
}

static void oki_tick(void)
{
    for (unsigned i = 0; i < 3; i++) {
        physical_t *v = &voices[i];
        if (!v->playing) continue;
        if (v->position == v->asset.frames) {
            event("natural_end", i, v->bank, v->sample, v->volume);
            *v = (physical_t){0};
            natural_ends++;
            continue;
        }
        unsigned offset = v->asset.offset + 2 * v->position++;
        unsigned value = (unsigned)pcm[offset] * 256 + pcm[offset + 1];
        v->current = value & 0x8000 ? (int)value - 65536 : (int)value;
    }
}

static int finished(void)
{
    for (unsigned i = 0; i < 3; i++)
        if (voices[i].playing || wf_sfx_voice(i)->active) return 0;
    return 1;
}

int main(int argc, char **argv)
{
    assert(argc == 5);
    unsigned command = (unsigned)strtoul(argv[2], NULL, 0);
    assert(command >= 0x20 && command <= 0x81);
    FILE *in = fopen(argv[1], "rb");
    assert(in && !fseek(in, 0, SEEK_END));
    long size = ftell(in);
    assert(size > 0);
    rewind(in);
    pcm = malloc((size_t)size);
    assert(pcm && fread(pcm, 1, (size_t)size, in) == (size_t)size);
    fclose(in);
    for (unsigned i = 0; i < 114; i++) {
        const fixture_t *f = &fixtures[i];
        assert((uint64_t)f->offset + f->frames * 2 <= (uint64_t)size);
        assets[f->bank][f->id] = (asset_t){f->offset, f->frames};
    }
    FILE *out = fopen(argv[3], "wb");
    events = fopen(argv[4], "w");
    assert(out && events);
    fputs("time_numerator,time_denominator,action,voice,bank,sample,volume\n", events);
    wf_sfx_runtime_reset();
    time_den = 1;
    int allocated = wf_sfx_start_voice(command, play, stop, NULL);
    assert(allocated >= 0);
    static const unsigned volume[16] = {32,22,16,11,8,6,4,3,2,0,0,0,0,0,0,0};
    uint64_t next_oki = 1, next_timer = 1, frames = 0, finished_frame = UINT64_MAX;
    unsigned peak = 0, clipped = 0;
    for (; frames < 60ull * 44100; frames++) {
        uint64_t sample_time = frames + 1;
        for (;;) {
            int oki_due = next_oki * 44100 <= sample_time * 8000;
            int timer_due = next_timer * (64 * 253) * 44100 <= sample_time * 3579545;
            if (!oki_due && !timer_due) break;
            if (timer_due && (!oki_due || next_timer * (64 * 253) * 8000 <= next_oki * 3579545)) {
                time_num = next_timer * (64 * 253); time_den = 3579545;
                wf_sfx_timer_a_tick(play, stop, NULL);
                next_timer++;
            } else {
                time_num = next_oki; time_den = 8000;
                oki_tick();
                next_oki++;
            }
        }
        int64_t mix = 0;
        for (unsigned i = 0; i < 3; i++)
            mix += voices[i].current * (int)volume[voices[i].volume] / 32;
        mix = (mix * 21627) >> 15;
        if (mix > 32767) { mix = 32767; clipped++; }
        if (mix < -32768) { mix = -32768; clipped++; }
        unsigned amplitude = (unsigned)(mix < 0 ? -mix : mix);
        if (amplitude > peak) peak = amplitude;
        uint16_t word = (uint16_t)(int16_t)mix;
        unsigned char output[2] = {(unsigned char)word, (unsigned char)(word >> 8)};
        assert(fwrite(output, 1, 2, out) == 2);
        if (finished_frame == UINT64_MAX && finished()) finished_frame = frames;
        if (finished_frame != UINT64_MAX && frames - finished_frame >= 11025) { frames++; break; }
    }
    assert(finished_frame != UINT64_MAX);
    fclose(events); fclose(out); free(pcm);
    printf("{\"command\":%u,\"frames\":%llu,\"model_complete_frame\":%llu,"
           "\"appended_silence_frames\":11025,\"peak\":%u,\"clipped\":%u,"
           "\"starts\":%u,\"suppressed_starts\":%u,\"stops\":%u,\"natural_ends\":%u}\n",
           command, (unsigned long long)frames, (unsigned long long)finished_frame,
           peak, clipped, starts, suppressed, stops, natural_ends);
    return 0;
}
