#ifndef WP0_RICE_H
#define WP0_RICE_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

enum { WP0_PCM_RATE=44100, WP0_PCM_BLOCK=1024,
       WP0_RICE_HEADER_BYTES=32, WP0_RICE_BLOCK_HEADER=8,
       WP0_RICE_STAGING_BYTES=8+2*WP0_PCM_BLOCK };
typedef bool (*wp0_audio_read_fn)(uint32_t offset, void *dst,
                                uint32_t bytes, void *user);
typedef struct {
    wp0_audio_read_fn read;
    void *user;
    uint32_t file_bytes,total_samples,block_count,pcm_fnv1a;
    uint32_t offset,next_block,decoded_samples;
    uint8_t staging[WP0_RICE_STAGING_BYTES];
} wp0_rice_stream_t;

/* Output words are native signed16. The SH2 is big-endian, so those words may
 * be copied directly to SCSP PCM16 memory. Every source access is bounded. */
bool wp0_rice_decode_block(const uint8_t *src, size_t bytes,
                          int16_t *out, size_t capacity, unsigned *samples);
bool wp0_rice_open(wp0_rice_stream_t *s, wp0_audio_read_fn read,
                   void *user, uint32_t file_bytes);
bool wp0_rice_next(wp0_rice_stream_t *s, int16_t out[WP0_PCM_BLOCK],
                   unsigned *samples);
#endif
