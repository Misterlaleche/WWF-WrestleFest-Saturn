#ifndef WP0_ATTRACT_PACKET_H
#define WP0_ATTRACT_PACKET_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define WP0_ATTRACT_HEADER_BYTES 64u
#define WP0_ATTRACT_RETAIN 0xffffu
#define WP0_ATTRACT_PCM_ZERO 0x0001u

typedef struct {
    uint32_t output_frame,source_frame,source_pc,pcm_offset,logic;
    uint32_t line_runs;
    uint16_t priority,scene,flags,scroll[4];
    uint16_t texture_runs,map_runs,palette_count,sprite_count,pcm_samples;
} wp0_attract_meta_t;

/* This is a trusted prepared view, not a seal for arbitrary caller-built data.
 * Only prepare may establish it. Keep both the view and its packet immutable
 * for its lifetime, and keep the packet storage alive throughout every use. */
typedef struct {
    const uint8_t *data;
    uint32_t bytes;
    wp0_attract_meta_t meta;
    bool valid;
} wp0_attract_prepared_t;

typedef struct {
    void *context;
    bool (*texture)(void *,uint16_t kind,uint16_t slot,uint16_t count,const uint8_t *raw);
    bool (*map)(void *,uint16_t kind,uint16_t first,uint16_t count,const uint8_t *raw);
    bool (*palette)(void *,uint16_t index,uint16_t rgb444);
    /* count==0 clears; count==0xffff retains, with raw==NULL. Raw records are
     * big-endian HhhH, including signed x/y and tile==0xffff clip commands. */
    bool (*sprites)(void *,uint16_t count,const uint8_t *raw);
    bool (*lines)(void *,uint16_t kind,uint16_t first,uint16_t count,int16_t x,int16_t y_at_first);
    /* raw==NULL with samples>0 means exact digital-zero samples (flag bit0);
     * those samples occupy no bytes in the packet. */
    bool (*pcm)(void *,uint32_t sample_offset,uint16_t samples,const uint8_t *raw);
} wp0_attract_packet_ops_t;

/* Whole packets must be 4-byte aligned and remain immutable through dispatch.
 * Raw callback pointers are big-endian source bytes, valid only for this call.
 * Present PCM starts at least 2-byte aligned; flag bit0 omits zero PCM entirely.
 * A zero sample count requires that flag to be clear. CRC32 covers the body.
 * Metadata is written only after success; malformed input never calls ops.
 * All callbacks are required. A callback failure stops immediately and returns
 * false; earlier side effects are not rolled back, so the caller must stop the
 * scene. Neither function allocates memory or invokes a hardware API. */
bool wp0_attract_packet_validate(const void *,uint32_t bytes,bool check_crc,
                                 wp0_attract_meta_t *metadata);
bool wp0_attract_packet_dispatch(const void *,uint32_t bytes,bool check_crc,
                                 const wp0_attract_packet_ops_t *,wp0_attract_meta_t *metadata);
/* Full validation (and optional CRC) occurs here, normally while hidden. A
 * failed prepare invalidates any previous view. View storage must not overlap
 * packet storage. No input byte or callback is modified/invoked by prepare. */
bool wp0_attract_packet_prepare(wp0_attract_prepared_t *,const void *,uint32_t bytes,bool check_crc);
/* For immutable packets prepared above only. Performs basic argument/view
 * sanity checks then the same callback core as normal dispatch, without
 * rescanning packet records. Normal dispatch remains fully validating. */
bool wp0_attract_packet_dispatch_prepared(const wp0_attract_prepared_t *,
                                         const wp0_attract_packet_ops_t *,wp0_attract_meta_t *metadata);
#endif
