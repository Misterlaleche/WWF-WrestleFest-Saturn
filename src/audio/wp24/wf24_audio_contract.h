#ifndef WF24_AUDIO_CONTRACT_H
#define WF24_AUDIO_CONTRACT_H

#include <stdbool.h>
#include <stdint.h>

typedef enum {
    WF24_ROUTE_STOP = 0,
    WF24_ROUTE_MUSIC,
    WF24_ROUTE_REGISTERED_NO_PRODUCER,
    WF24_ROUTE_SPECIAL_FADE,
    WF24_ROUTE_SFX,
    WF24_ROUTE_INVALID
} wf24_route_t;

typedef enum {
    WF24_FINITE = 1,
    WF24_CONTINUOUS = 2
} wf24_lifetime_t;

typedef struct {
    uint8_t command;
    uint8_t lifetime;
    uint8_t combined_ym_oki_verified;
    uint8_t bank0_assertion_verified;
    uint32_t source_offset_bytes;
    uint32_t total_samples;
    /* Must remain zero. The arcade contract has no short PCM loop object. */
    uint32_t pcm_loop_start_sample;
    uint32_t pcm_loop_samples;
} wf24_music_asset_t;

enum {
    WF24_SOUND_RAM_BASE = 0x25A00000u,
    WF24_MUSIC_RING_OFFSET = 0x00060000u,
    WF24_MUSIC_RING_BYTES = 0x00010000u,
    WF24_MUSIC_HALF_BYTES = 0x00008000u,
    WF24_MUSIC_RING_SAMPLES = 32768u,
    WF24_MUSIC_HALF_SAMPLES = 16384u,
    WF24_MUSIC_HALF_US = 371519u,
    WF24_MUSIC_SLOT = 31u,
    WF24_MUSIC_LSA = 0u,
    WF24_MUSIC_LEA = 32767u,
    WF24_MUSIC_RATE_HZ = 44100u,
    WF24_MUSIC_CHANNELS = 1u,
    WF24_MUSIC_BITS = 16u
};

typedef enum {
    WF24_PCM16_BIG_ENDIAN = 1
} wf24_pcm_endian_t;

typedef struct {
    uint32_t slot;
    uint32_t sound_ram_offset;
    uint32_t ring_samples;
    uint32_t lsa;
    uint32_t lea;
    uint32_t rate_hz;
    uint8_t channels;
    uint8_t bits;
    uint8_t signed_pcm;
    uint8_t big_endian;
    uint8_t centered_mono;
    uint8_t unity_stream_gain;
} wf24_slot31_format_t;

wf24_route_t wf24_route_command(uint8_t command);
bool wf24_music_asset_validate(const wf24_music_asset_t *asset);
void wf24_slot31_exact_format(wf24_slot31_format_t *format);
bool wf24_ca_valid(unsigned ca);
unsigned wf24_active_half(unsigned ca);

#endif
