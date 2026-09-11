#include <assert.h>
#include <stddef.h>
#include <stdint.h>

#include "../../../src/audio/wp24/wf24_runtime_audio.h"

typedef struct {
    unsigned stop, music, fade, sfx;
    uint8_t last;
} fake_t;

static bool stop_cb(void *u) { ((fake_t *)u)->stop++; return true; }

static bool music_cb(uint8_t c, void *u)
{
    fake_t *f = u;
    f->music++;
    f->last = c;
    return true;
}

static bool fade_cb(void *u) { ((fake_t *)u)->fade++; return true; }

static bool sfx_cb(uint8_t c, void *u)
{
    fake_t *f = u;
    f->sfx++;
    f->last = c;
    return true;
}

int main(void)
{
    static const uint16_t expected_stage_words[10] = {
        0x3104, 0x310A, 0x310C, 0x310F, 0x3106,
        0x3104, 0x310A, 0x310C, 0x310F, 0x3106
    };
    static const wf24_rom_audio_producer_t producers[] = {
        {0x0152BAu, 0x32u},
        {0x016DBEu, 0x32u},
        {0x01755Au, 0x32u},
        {0x017874u, 0x2Bu},
        {0x01787Eu, 0x32u},
        {0x0164B8u, 0x2Bu},
        {0x0164C2u, 0x32u}
    };
    fake_t f = {0};
    wf24_command_bus_t bus;
    wf24_command_bus_ops_t ops = {
        stop_cb, music_cb, fade_cb, sfx_cb, &f
    };
    uint8_t c = 0;
    size_t i;

    assert(wf24_command_bus_init(&bus, &ops));

    for (i = 0; i < 10; ++i)
        assert(wf24_stage_music_raw_word((unsigned)i) ==
               expected_stage_words[i]);
    assert(wf24_stage_music_raw_word(10) == 0);
    assert(wf24_runtime_dispatch_stage(&bus, 3) == WF24_DISPATCH_OK);
    assert(f.music == 1 && f.last == 0x0F);

    assert(wf24_ringout_producer_command(0x01FE44, 0x3165, &c) &&
           c == 0x65);
    assert(wf24_ringout_producer_command(0x01FE44, 0x3168, &c) &&
           c == 0x68);
    assert(!wf24_ringout_producer_command(0x01FE40, 0x3165, &c));
    assert(!wf24_ringout_producer_command(0x01FE44, 0x3164, &c));
    assert(!wf24_ringout_producer_command(0x01FE44, 0x3169, &c));
    assert(wf24_runtime_dispatch_ringout(&bus, 0x01FE44, 0x3167) ==
           WF24_DISPATCH_OK);
    assert(f.sfx == 1 && f.last == 0x67);

    assert(wf24_low_stamina_producer_command(0x0263CC, &c) &&
           c == 0x0B);
    assert(!wf24_low_stamina_producer_command(0x0263D0, &c));
    assert(wf24_runtime_dispatch_producer(&bus, 0x0263CC) ==
           WF24_DISPATCH_OK);
    assert(f.music == 2 && f.last == 0x0B);

    for (i = 0; i < sizeof(producers) / sizeof(producers[0]); ++i) {
        assert(wf24_finisher_producer_command(producers[i].producer_pc, &c));
        assert(c == producers[i].command);
    }
    assert(!wf24_finisher_producer_command(0x01434A, &c));

    assert(wf24_runtime_dispatch_producer(&bus, 0x016DBE) ==
           WF24_DISPATCH_OK);
    assert(f.sfx == 2 && f.last == 0x32);

    return 0;
}
