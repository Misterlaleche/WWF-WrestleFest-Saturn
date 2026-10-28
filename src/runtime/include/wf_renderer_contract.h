#ifndef WF_RENDERER_CONTRACT_H
#define WF_RENDERER_CONTRACT_H

#include <stdint.h>

#define WF_VIEW_W 320
#define WF_VIEW_H 240
#define WF_WORLD_W 512
#define WF_WORLD_H 512
#define WF_SPRITE_RECORD_BYTES 16
#define WF_SPRITE_RECORDS_MAX 512
#define WF_SPRITE_CHAIN_MAX 7

#define WF_PRI_78 0x78
#define WF_PRI_7B 0x7B
#define WF_PRI_7C 0x7C

typedef enum wf_layer {
    WF_LAYER_FG_WORLD,
    WF_LAYER_BG_WORLD,
    WF_LAYER_SPRITES,
    WF_LAYER_TEXT
} wf_layer_t;

static inline int wf_sprite_x_decode(unsigned raw_x) {
    raw_x &= 0x1ffU;
    return raw_x > 496U ? (int)raw_x - 512 : (int)raw_x;
}

static inline int wf_sprite_y_decode(unsigned raw_y) {
    raw_y &= 0x1ffU;
    return (int)((256U - raw_y) & 0x1ffU) - 16;
}

static inline unsigned wf_bit_width(unsigned x) {
    unsigned n = 0;
    while (x) { ++n; x >>= 1; }
    return n;
}

static inline unsigned wf_sprite_number_align(unsigned number, unsigned chain) {
    unsigned bits = wf_bit_width(chain & 7U);
    unsigned mask = bits ? ~((1U << bits) - 1U) : ~0U;
    return number & mask;
}

/* xBGR444 -> 0x00RRGGBB. */
static inline uint32_t wf_xbgr444_to_rgb888(uint16_t c) {
    unsigned r4 = c & 0x000fU;
    unsigned g4 = (c >> 4) & 0x000fU;
    unsigned b4 = (c >> 8) & 0x000fU;
    unsigned r8 = (r4 << 4) | r4;
    unsigned g8 = (g4 << 4) | g4;
    unsigned b8 = (b4 << 4) | b4;
    return (r8 << 16) | (g8 << 8) | b8;
}

#endif
