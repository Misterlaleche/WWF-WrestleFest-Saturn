#include <assert.h>
#include <stdint.h>
#include <string.h>

#include "../../../src/audio/wp24/wf24_audio_contract.h"
#include "../../../src/audio/wp24/wf24_command_bus.h"

typedef struct {
    unsigned stop, music, fade, sfx;
    uint8_t last;
    bool result;
} fake_t;

static bool stop_cb(void *u)
{
    fake_t *f = u;
    ++f->stop;
    return f->result;
}

static bool music_cb(uint8_t c, void *u)
{
    fake_t *f = u;
    ++f->music;
    f->last = c;
    return f->result;
}

static bool fade_cb(void *u)
{
    fake_t *f = u;
    ++f->fade;
    return f->result;
}

static bool sfx_cb(uint8_t c, void *u)
{
    fake_t *f = u;
    ++f->sfx;
    f->last = c;
    return f->result;
}

int main(void)
{
    fake_t f = {0};
    wf24_command_bus_t bus;
    wf24_command_bus_t zero_bus;
    wf24_command_bus_ops_t ops = {
        stop_cb, music_cb, fade_cb, sfx_cb, &f
    };

    memset(&zero_bus, 0, sizeof(zero_bus));
    assert(wf24_command_dispatch(NULL, 0x04) == WF24_DISPATCH_BACKEND_ERROR);
    assert(wf24_command_dispatch(&zero_bus, 0x04) ==
           WF24_DISPATCH_BACKEND_ERROR);

    f.result = true;
    assert(wf24_command_bus_init(&bus, &ops));

    assert(wf24_command_dispatch(&bus, 0x00) == WF24_DISPATCH_OK);
    assert(f.stop == 1);

    assert(wf24_command_dispatch(&bus, 0x01) == WF24_DISPATCH_OK);
    assert(wf24_command_dispatch(&bus, 0x12) == WF24_DISPATCH_OK);
    assert(f.music == 2 && f.last == 0x12);

    assert(wf24_command_dispatch(&bus, 0x13) ==
           WF24_DISPATCH_NO_PRODUCER);
    assert(wf24_command_dispatch(&bus, 0x1E) ==
           WF24_DISPATCH_NO_PRODUCER);

    assert(wf24_command_dispatch(&bus, 0x1F) == WF24_DISPATCH_OK);
    assert(f.fade == 1);

    assert(wf24_command_dispatch(&bus, 0x20) == WF24_DISPATCH_OK);
    assert(wf24_command_dispatch(&bus, 0x81) == WF24_DISPATCH_OK);
    assert(f.sfx == 2 && f.last == 0x81);

    assert(wf24_command_dispatch(&bus, 0x82) == WF24_DISPATCH_INVALID);
    assert(bus.rejected_count == 3);

    f.result = false;
    assert(wf24_command_dispatch(&bus, 0x04) ==
           WF24_DISPATCH_BACKEND_ERROR);

    return 0;
}
