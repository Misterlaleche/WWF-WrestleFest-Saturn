#ifndef WP02_BGM_CODEC_H
#define WP02_BGM_CODEC_H
#include <stdbool.h>
#include <stdint.h>

enum {
    WP02_BGM_HEADER_BYTES = 32,
    WP02_BGM_BLOCK_HEADER_BYTES = 12,
    WP02_BGM_BLOCK_SAMPLES = 1024,
    WP02_BGM_BLOCK_MAX_BYTES = WP02_BGM_BLOCK_HEADER_BYTES + WP02_BGM_BLOCK_SAMPLES * 2
};
typedef struct {
    uint32_t samples;
    uint32_t blocks;
    uint32_t payload_bytes;
} wp02_bgm_header_t;
bool wp02_bgm_header_read(const uint8_t *src, unsigned bytes, wp02_bgm_header_t *out);
bool wp02_bgm_block_decode(const uint8_t *src, unsigned bytes,
                          uint8_t *dst_be, unsigned capacity,
                          unsigned *consumed, unsigned *samples);
#endif
