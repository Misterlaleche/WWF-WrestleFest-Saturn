#include "wf_scsp_sfx.h"
#include <assert.h>
static bool write_word(uint32_t address,uint16_t value,void *user)
{(void)address;(void)value;(void)user;return true;}

int main(void)
{
    wf_scsp_sfx_t idle = {0};
    idle.bound = 1;
    idle.frames = 10;
    wf_scsp_sfx_advance(&idle, 513);
    assert(idle.frames == 523);
    assert(idle.fault == 0);
    assert(idle.underruns == 0);

    wf_scsp_sfx_t active = {0};
    active.bound = 1;
    active.write16 = write_word;
    active.pcm[0].playing = 1;
    wf_scsp_sfx_advance(&active, 513);
    assert(active.fault == 0);
    assert(active.underruns == 1);
    assert(active.pcm[0].playing == 0);

    wf_scsp_sfx_t delayed = {0};
    delayed.bound = 1;
    delayed.write16 = write_word;
    delayed.sequence[0].active = 1;
    wf_scsp_sfx_advance(&delayed, 513);
    assert(delayed.fault == 0);
    assert(delayed.underruns == 1);
    assert(delayed.sequence[0].active == 0);

    wf_scsp_sfx_t shortfall = {0};
    shortfall.bound = 1;
    shortfall.write16 = write_word;
    shortfall.pcm[0].playing = 1;
    shortfall.pcm[0].initial_fill = 1;
    shortfall.pcm[0].total_frames = 1000;
    shortfall.pcm[0].write_at = 64;
    shortfall.pcm[1].playing = 1;
    shortfall.pcm[1].last_frame = 500;
    shortfall.pcm[1].total_frames = 1000;
    shortfall.pcm[1].write_at = 128;
    wf_scsp_sfx_advance(&shortfall, 400);
    assert(shortfall.fault == 0);
    assert(shortfall.underruns == 1);
    assert(shortfall.pcm[0].playing == 0);
    assert(shortfall.pcm[1].playing == 1);
    return 0;
}
