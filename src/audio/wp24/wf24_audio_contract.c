#include "wf24_audio_contract.h"

#include <string.h>

wf24_route_t
wf24_route_command(uint8_t c)
{
    if (c == 0x00u)
        return WF24_ROUTE_STOP;
    if (c >= 0x01u && c <= 0x12u)
        return WF24_ROUTE_MUSIC;
    if (c >= 0x13u && c <= 0x1Eu)
        return WF24_ROUTE_REGISTERED_NO_PRODUCER;
    if (c == 0x1Fu)
        return WF24_ROUTE_SPECIAL_FADE;
    if (c >= 0x20u && c <= 0x81u)
        return WF24_ROUTE_SFX;
    return WF24_ROUTE_INVALID;
}

bool
wf24_music_asset_validate(const wf24_music_asset_t *a)
{
    if (!a || a->command < 0x01u || a->command > 0x12u)
        return false;
    if (a->lifetime != WF24_FINITE && a->lifetime != WF24_CONTINUOUS)
        return false;
    if (!a->combined_ym_oki_verified || !a->bank0_assertion_verified)
        return false;
    if (!a->total_samples)
        return false;
    if (a->pcm_loop_start_sample != 0u || a->pcm_loop_samples != 0u)
        return false;
    return true;
}

void
wf24_slot31_exact_format(wf24_slot31_format_t *f)
{
    if (!f)
        return;
    memset(f, 0, sizeof(*f));
    f->slot = WF24_MUSIC_SLOT;
    f->sound_ram_offset = WF24_MUSIC_RING_OFFSET;
    f->ring_samples = WF24_MUSIC_RING_SAMPLES;
    f->lsa = WF24_MUSIC_LSA;
    f->lea = WF24_MUSIC_LEA;
    f->rate_hz = WF24_MUSIC_RATE_HZ;
    f->channels = WF24_MUSIC_CHANNELS;
    f->bits = WF24_MUSIC_BITS;
    f->signed_pcm = 1u;
    f->big_endian = 1u;
    f->centered_mono = 1u;
    f->unity_stream_gain = 1u;
}

bool
wf24_ca_valid(unsigned ca)
{
    return ca <= 7u;
}

unsigned
wf24_active_half(unsigned ca)
{
    return ca < 4u ? 0u : 1u;
}
