#ifndef WP0_AUDIO_PLAYER_H
#define WP0_AUDIO_PLAYER_H
#include "wp0_rice.h"
enum { WP0_AUDIO_RING_SAMPLES=32768,WP0_AUDIO_RING_BYTES=65536,
       WP0_AUDIO_REGION_SAMPLES=4096,WP0_AUDIO_RING_REGIONS=8 };
typedef enum { WP0_AUDIO_EMPTY,WP0_AUDIO_PREPARED,WP0_AUDIO_PLAYING,
               WP0_AUDIO_STOPPED,WP0_AUDIO_ERROR } wp0_audio_state_t;
typedef enum { WP0_AUDIO_OK,WP0_AUDIO_BAD_SOURCE,WP0_AUDIO_BAD_PLATFORM,
               WP0_AUDIO_SERVICE_GAP,WP0_AUDIO_UNDERRUN } wp0_audio_error_t;
typedef struct {
    bool (*write_pcm)(unsigned sample_offset,const int16_t *samples,
                      unsigned count,void *user);
    unsigned (*read_region)(void *user);
    bool (*key_on)(void *user);
    void (*key_off)(void *user);
    void *user;
} wp0_audio_ops_t;
typedef struct {
    wp0_rice_stream_t stream;
    wp0_audio_ops_t ops;
    int16_t pcm[WP0_PCM_BLOCK];
    uint32_t queued_samples,consumed_lower_bound,service_calls,decoded_blocks;
    unsigned previous_region;
    wp0_audio_state_t state;
    wp0_audio_error_t error;
    /* Observation only. UINT32_MAX means this service did not reach the
     * monitor read or advance calculation, respectively. */
    uint32_t last_region,last_advance,last_service_us,max_service_us;
} wp0_audio_player_t;

/* Prepare before timing-sensitive display: primes the complete64KiB ring. */
bool wp0_audio_prepare(wp0_audio_player_t *p,wp0_audio_read_fn read,
                      void *source_user,uint32_t source_bytes,
                      const wp0_audio_ops_t *ops);
/* Exactly one start per prepare. Replaying requires another prepare. */
bool wp0_audio_start(wp0_audio_player_t *p);
void wp0_audio_stop(wp0_audio_player_t *p);
/* At most1024 decoded samples/call. No allocation, CD, waits, or DMA. */
bool wp0_audio_service(wp0_audio_player_t *p,uint32_t elapsed_us);

/* Native backend owns slot0 and all SCSP key operations; sound CPU stays off.
 * Initialize BEFORE storing compressed data in sound RAM. The backend itself
 * writes the ring through0x0FFFF plus its two-byte interpolation guard
 * at0x10000..0x10001. Compressed storage starts at0x10002. */
void wp0_scsp_initialize(void);
wp0_audio_ops_t wp0_scsp_ops(void);
#endif
