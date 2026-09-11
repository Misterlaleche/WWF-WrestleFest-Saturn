#pragma once
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define WF_ATTRACT_BIO_STATE_FIRST 0x02U
#define WF_ATTRACT_BIO_STATE_LAST  0x0DU
#define WF_ATTRACT_BIO_FIGHTERS 12U
#define WF_ATTRACT_BIO_FRAMES 3U
#define WF_ATTRACT_BIO_PORTRAIT_W 80U
#define WF_ATTRACT_BIO_PORTRAIT_H 80U
#define WF_ATTRACT_BIO_TEXT_W 224U
#define WF_ATTRACT_BIO_TEXT_H 80U
#define WF_ATTRACT_BIO_PORTRAIT_FRAME_BYTES (WF_ATTRACT_BIO_PORTRAIT_W*WF_ATTRACT_BIO_PORTRAIT_H)
#define WF_ATTRACT_BIO_PORTRAIT_BYTES (WF_ATTRACT_BIO_FRAMES*WF_ATTRACT_BIO_PORTRAIT_FRAME_BYTES)
#define WF_ATTRACT_BIO_TEXT_BYTES (WF_ATTRACT_BIO_TEXT_W*WF_ATTRACT_BIO_TEXT_H)
#define WF_ATTRACT_BIO_TEXTURE_BYTES (WF_ATTRACT_BIO_PORTRAIT_BYTES+WF_ATTRACT_BIO_TEXT_BYTES)
#define WF_ATTRACT_BIO_PALETTE_BYTES 512U
#define WF_ATTRACT_BIO_PACK_HEADER_BYTES 64U
#define WF_ATTRACT_BIO_PACK_BYTES (WF_ATTRACT_BIO_PACK_HEADER_BYTES+WF_ATTRACT_BIO_TEXTURE_BYTES+WF_ATTRACT_BIO_PALETTE_BYTES)

#define WF_ATTRACT_BIO_PORTRAIT_X 7
#define WF_ATTRACT_BIO_PORTRAIT_Y (-1)
#define WF_ATTRACT_BIO_TEXT_X 96
#define WF_ATTRACT_BIO_TEXT_Y 8
#define WF_ATTRACT_BIO_ANIM_END_TICK 0x0100U
#define WF_ATTRACT_BIO_CLEANUP_TICK 0x0200U
#define WF_ATTRACT_BIO_INITIAL_DWELL 8U
#define WF_ATTRACT_BIO_DWELL_MIN 8U
#define WF_ATTRACT_BIO_DWELL_MAX 23U

typedef enum {
 WF_BIO_HOGAN=0,
 WF_BIO_WARRIOR=1,
 WF_BIO_JAKE=2,
 WF_BIO_BOSSMAN=3,
 WF_BIO_HAWK=4,
 WF_BIO_ANIMAL=5,
 WF_BIO_SLAUGHTER=6,
 WF_BIO_PERFECT=7,
 WF_BIO_EARTHQUAKE=8,
 WF_BIO_DIBIASE=9,
 WF_BIO_SMASH=10,
 WF_BIO_CRUSH=11
} wf_attract_bio_fighter_t;

typedef struct {
 uint8_t attract_state;
 uint8_t fighter;
 uint8_t frame;
 uint8_t dwell;
 uint8_t toggle;
 bool initialized;
 bool cleaned;
 bool draw_portrait;
 bool draw_text;
} wf_attract_bio_t;

typedef struct {
 const uint8_t *portrait[WF_ATTRACT_BIO_FRAMES];
 const uint8_t *text;
 const uint8_t *palette_be;
 uint8_t fighter;
 uint32_t payload_hash;
} wf_attract_bio_pack_t;

bool wf_attract_bio_fighter_for_state(unsigned attract_state,unsigned *fighter);
void wf_attract_bio_begin(wf_attract_bio_t *bio,unsigned attract_state);
void wf_attract_bio_step(wf_attract_bio_t *bio,uint16_t global_demo_tick,uint8_t rng_nibble);
unsigned wf_attract_bio_frame(const wf_attract_bio_t *bio);
bool wf_attract_bio_portrait_visible(const wf_attract_bio_t *bio);
bool wf_attract_bio_text_visible(const wf_attract_bio_t *bio);
unsigned wf_attract_bio_dwell_from_nibble(uint8_t rng_nibble);
uint32_t wf_attract_bio_fnv1a(const uint8_t *data,size_t bytes);
bool wf_attract_bio_pack_open(wf_attract_bio_pack_t *out,const uint8_t *data,size_t bytes,unsigned expected_fighter);
