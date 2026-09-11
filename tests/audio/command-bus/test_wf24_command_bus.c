#include <assert.h>
#include <stdint.h>

#include "../../../src/audio/wp24/wf24_audio_contract.h"
#include "../../../src/audio/wp24/wf24_command_bus.h"
#include "../../../src/audio/wp24/wf24_game_bindings.h"

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

    assert(wf24_command_dispatch(&bus, 0x00) == WF24_DISPATCH_OK);
    assert(f.stop == 1);

    assert(wf24_command_dispatch(&bus, 0x04) == WF24_DISPATCH_OK);
    assert(f.music == 1 && f.last == 0x04);

    assert(wf24_command_dispatch(&bus, 0x1F) == WF24_DISPATCH_OK);
    assert(f.fade == 1);

    assert(wf24_command_dispatch(&bus, 0x20) == WF24_DISPATCH_OK);
    assert(f.sfx == 1 && f.last == 0x20);

    assert(wf24_command_dispatch(&bus, 0x13) == WF24_DISPATCH_NO_PRODUCER);
    assert(wf24_command_dispatch(&bus, 0x82) == WF24_DISPATCH_INVALID);

    assert(wf24_stage_music_command(0) == 0x04);
    assert(wf24_stage_music_command(9) == 0x06);
    assert(wf24_stage_music_command(10) == 0x00);

    assert(wf24_low_stamina_command(0x17, &c) && c == 0x0B);
    assert(!wf24_low_stamina_command(0x18, &c));

    for (unsigned n = 17; n <= 20; ++n) {
        assert(wf24_countout_command(n, &c));
        assert(c == (uint8_t)(0x65 + n - 17));
    }
    assert(!wf24_countout_command(16, &c));
    assert(!wf24_countout_command(21, &c));

    assert(wf24_finisher_heavy_command() == 0x32);
    assert(wf24_heavy_short_layer_command() == 0x2B);
    return 0;
}
