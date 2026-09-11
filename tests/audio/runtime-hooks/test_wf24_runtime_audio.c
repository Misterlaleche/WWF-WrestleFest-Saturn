#include <assert.h>
#include <stdint.h>

#include "../../../src/audio/wp24/wf24_runtime_audio.h"

typedef struct {
    unsigned stop, music, fade, sfx;
    uint8_t last;
} fake_t;

static bool stop_cb(void *u) { ((fake_t *)u)->stop++; return true; }
static bool music_cb(uint8_t c, void *u)
{
    fake_t *f = u; f->music++; f->last = c; return true;
}
static bool fade_cb(void *u) { ((fake_t *)u)->fade++; return true; }
static bool sfx_cb(uint8_t c, void *u)
{
    fake_t *f = u; f->sfx++; f->last = c; return true;
}

int main(void)
{
    fake_t f = {0};
    wf24_command_bus_t bus;
    wf24_command_bus_ops_t ops = {
        stop_cb, music_cb, fade_cb, sfx_cb, &f
    };
    uint8_t c = 0;

    assert(wf24_command_bus_init(&bus, &ops));

    assert(wf24_stage_music_raw_word(0) == 0x3104);
    assert(wf24_stage_music_raw_word(9) == 0x3106);
    assert(wf24_stage_music_raw_word(10) == 0);
    assert(wf24_runtime_dispatch_stage(&bus, 0) == WF24_DISPATCH_OK);
    assert(f.music == 1 && f.last == 0x04);

    assert(wf24_ringout_raw54_command(0x3165, &c) && c == 0x65);
    assert(wf24_ringout_raw54_command(0x3168, &c) && c == 0x68);
    assert(!wf24_ringout_raw54_command(0x3164, &c));
    assert(!wf24_ringout_raw54_command(0x3169, &c));
    assert(wf24_runtime_dispatch_raw54(&bus, 0x3167) == WF24_DISPATCH_OK);
    assert(f.sfx == 1 && f.last == 0x67);

    assert(wf24_low_stamina_callsite(0x0263CC, &c) && c == 0x0B);
    assert(!wf24_low_stamina_callsite(0x0263CE, &c));
    assert(wf24_runtime_dispatch_callsite(&bus, 0x0263CC) ==
           WF24_DISPATCH_OK);
    assert(f.music == 2 && f.last == 0x0B);

    assert(wf24_finisher_callsite_command(0x0152BA, &c) && c == 0x32);
    assert(wf24_finisher_callsite_command(0x017874, &c) && c == 0x2B);
    assert(wf24_finisher_callsite_command(0x01787E, &c) && c == 0x32);
    assert(wf24_finisher_callsite_command(0x0164B8, &c) && c == 0x2B);
    assert(wf24_finisher_callsite_command(0x0164C2, &c) && c == 0x32);
    assert(!wf24_finisher_callsite_command(0x01434A, &c));

    assert(wf24_runtime_dispatch_callsite(&bus, 0x0152BA) ==
           WF24_DISPATCH_OK);
    assert(f.sfx == 2 && f.last == 0x32);
    return 0;
}
