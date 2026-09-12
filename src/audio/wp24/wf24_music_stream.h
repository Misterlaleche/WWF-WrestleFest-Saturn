#ifndef WF24_MUSIC_STREAM_H
#define WF24_MUSIC_STREAM_H

#include <stdbool.h>
#include <stdint.h>

#include "wf24_audio_contract.h"

typedef enum {
    WF24_STREAM_STOPPED = 0,
    WF24_STREAM_PRIMING,
    WF24_STREAM_PLAYING,
    WF24_STREAM_ERROR
} wf24_stream_state_t;

typedef enum {
    WF24_ERR_NONE = 0,
    WF24_ERR_BAD_ASSET,
    WF24_ERR_IO,
    WF24_ERR_BAD_CA,
    WF24_ERR_SERVICE_GAP,
    WF24_ERR_CONTINUOUS_SOURCE_EXHAUSTED,
    WF24_ERR_PLATFORM
} wf24_stream_error_t;

/*
 * Every slot-31 control callback must execute through the single Saturn sound
 * owner used alongside poneSound/M68K. The stream core never writes KYONEX.
 */
typedef struct {
    bool (*read_source_be)(uint32_t offset_bytes, void *dst,
                           uint32_t bytes, void *user);
    bool (*render_live_be)(void *dst, uint32_t bytes, void *user);
    bool (*copy_half_be)(unsigned half, const void *src_be,
                         uint32_t bytes, void *user);
    bool (*slot31_prepare_serialized)(const wf24_slot31_format_t *format,
                                      void *user);
    bool (*slot31_start_serialized)(void *user);
    bool (*slot31_stop_serialized)(void *user);
    bool (*slot31_read_ca_serialized)(unsigned *ca, void *user);
    void *user;
} wf24_stream_ops_t;

typedef struct {
    wf24_stream_ops_t ops;
    wf24_music_asset_t asset;
    uint8_t staging[WF24_MUSIC_HALF_BYTES];
    wf24_stream_state_t state;
    wf24_stream_error_t error;
    uint32_t source_sample;
    uint32_t refills;
    uint32_t service_gap_errors;
    uint32_t source_exhaustions;
    uint8_t current_command;
    uint8_t last_active_half;
    uint8_t eof_seen;
    uint8_t eof_half;
    uint8_t live_source;
} wf24_music_stream_t;

void wf24_stream_reset(wf24_music_stream_t *stream);
bool wf24_stream_init(wf24_music_stream_t *stream,
                      const wf24_stream_ops_t *ops);
bool wf24_stream_start(wf24_music_stream_t *stream,
                       const wf24_music_asset_t *asset);
bool wf24_stream_start_live(wf24_music_stream_t *stream, uint8_t command);
bool wf24_stream_stop(wf24_music_stream_t *stream);
bool wf24_stream_service(wf24_music_stream_t *stream, uint32_t delta_us);

#endif
