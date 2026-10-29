#ifndef WP01_OKI_H
#define WP01_OKI_H
#include <stdint.h>
#include <stddef.h>

#define WP01_OKI_SLOT_FIRST 24u
#define WP01_OKI_VOICES 4u
#define WP01_OKI_RAW_VOLUME 255u
#define WP01_OKI_PITCH_8000 0x69CEu

/* SCSP register offsets are relative to the register base, not RAM.
 * SH-2 base: 0x25B00000; 68000 base: 0x100000.
 * One owner must serialize all KYONEX operations across FM and PCM. */
typedef void (*wp01_oki_write16_fn)(void *user, uint16_t reg, uint16_t value);

typedef struct {
    uint32_t scsp_address;
    uint16_t samples;       /* LEA = sample count; require one zero guard. */
    uint8_t bank, sample_id;
    uint8_t baked_volume;   /* Original volume nibble, or RAW_VOLUME. */
} wp01_oki_asset_t;

typedef struct {
    uint32_t started_at, duration;
    uint16_t control;
    uint8_t active, sample_id, volume;
} wp01_oki_voice_t;

typedef struct {
    uint32_t starts, stops, natural_ends, ignored_busy;
    uint32_t missing_asset, invalid_asset, unsupported_bank;
} wp01_oki_stats_t;

typedef struct {
    const wp01_oki_asset_t *assets;
    size_t asset_count;
    wp01_oki_write16_fn write16;
    void *user;
    wp01_oki_voice_t voices[WP01_OKI_VOICES];
    wp01_oki_stats_t stats;
    int16_t pending_sample;
    uint8_t bank;
} wp01_oki_t;

/* Assets and one 16-bit zero guard per asset must already reside in SCSP RAM.
 * No allocation, CD I/O, PCM decoding, interrupt installation or global
 * SCSP initialization is performed by this module. */
void wp01_oki_init(wp01_oki_t *, const wp01_oki_asset_t *, size_t,
                  wp01_oki_write16_fn, void *user);
void wp01_oki_reset(wp01_oki_t *);
void wp01_oki_advance(wp01_oki_t *, uint32_t time_44100);
void wp01_oki_write(wp01_oki_t *, uint8_t data, uint32_t time_44100);
/* This isolated entrance supports bank zero only. A nonzero bank fails
 * closed; changing an active sample's ROM bank is not approximated. */
int wp01_oki_set_bank(wp01_oki_t *, unsigned bank);
unsigned wp01_oki_status(const wp01_oki_t *);
uint8_t wp01_oki_volume_tl(unsigned nibble);
#endif
