#include "wf24_saturn_copy.h"

#include <string.h>

#include "wf24_audio_contract.h"

bool
wf24_saturn_copy_half_be(unsigned half, const void *src_be,
                         uint32_t bytes, void *user)
{
    uintptr_t address;
    (void)user;

    if (!src_be || half > 1u || bytes != WF24_MUSIC_HALF_BYTES)
        return false;

    address = (uintptr_t)WF24_SOUND_RAM_BASE +
              (uintptr_t)WF24_MUSIC_RING_OFFSET +
              (uintptr_t)half * WF24_MUSIC_HALF_BYTES;
    memcpy((void *)address, src_be, bytes);
    return true;
}
