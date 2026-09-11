#include "wf24_music_stream.h"

#include <string.h>

static void
stream_error(wf24_music_stream_t *s, wf24_stream_error_t error)
{
    s->error = error;
    s->state = WF24_STREAM_ERROR;
}

static void
stop_slot_after_playback_error(wf24_music_stream_t *s)
{
    /* Best-effort fail-closed: never knowingly leave stale ring audio running. */
    if (s && s->ops.slot31_stop_serialized)
        (void)s->ops.slot31_stop_serialized(s->ops.user);
}

void
wf24_stream_reset(wf24_music_stream_t *s)
{
    if (!s)
        return;
    memset(s, 0, sizeof(*s));
    s->state = WF24_STREAM_STOPPED;
    s->last_active_half = 2u;
    s->eof_half = 2u;
}

bool
wf24_stream_init(wf24_music_stream_t *s, const wf24_stream_ops_t *ops)
{
    if (!s || !ops ||
        !ops->read_source_be ||
        !ops->copy_half_be ||
        !ops->slot31_prepare_serialized ||
        !ops->slot31_start_serialized ||
        !ops->slot31_stop_serialized ||
        !ops->slot31_read_ca_serialized)
        return false;

    wf24_stream_reset(s);
    s->ops = *ops;
    return true;
}

static bool
fill_half(wf24_music_stream_t *s, unsigned half)
{
    uint32_t remaining;
    uint32_t take;

    if (half > 1u) {
        stream_error(s, WF24_ERR_PLATFORM);
        return false;
    }

    if (s->source_sample >= s->asset.total_samples) {
        if (s->asset.lifetime == WF24_CONTINUOUS) {
            ++s->source_exhaustions;
            stream_error(s, WF24_ERR_CONTINUOUS_SOURCE_EXHAUSTED);
            return false;
        }

        memset(s->staging, 0, sizeof(s->staging));
        s->eof_seen = 1u;
        s->eof_half = (uint8_t)half;
    } else {
        remaining = s->asset.total_samples - s->source_sample;
        take = remaining < WF24_MUSIC_HALF_SAMPLES ?
            remaining : WF24_MUSIC_HALF_SAMPLES;

        if (s->asset.lifetime == WF24_CONTINUOUS &&
            take < WF24_MUSIC_HALF_SAMPLES) {
            ++s->source_exhaustions;
            stream_error(s, WF24_ERR_CONTINUOUS_SOURCE_EXHAUSTED);
            return false;
        }

        if (!s->ops.read_source_be(
                s->asset.source_offset_bytes + s->source_sample * 2u,
                s->staging,
                take * 2u,
                s->ops.user)) {
            stream_error(s, WF24_ERR_IO);
            return false;
        }

        s->source_sample += take;

        if (take < WF24_MUSIC_HALF_SAMPLES) {
            memset(s->staging + take * 2u, 0,
                   (WF24_MUSIC_HALF_SAMPLES - take) * 2u);
        }

        /* Exact-boundary EOF belongs to the half containing the final sample. */
        if (s->asset.lifetime == WF24_FINITE &&
            s->source_sample == s->asset.total_samples) {
            s->eof_seen = 1u;
            s->eof_half = (uint8_t)half;
        }
    }

    if (!s->ops.copy_half_be(half, s->staging,
                             WF24_MUSIC_HALF_BYTES, s->ops.user)) {
        stream_error(s, WF24_ERR_PLATFORM);
        return false;
    }

    ++s->refills;
    return true;
}

bool
wf24_stream_stop(wf24_music_stream_t *s)
{
    if (!s)
        return false;

    if (s->state != WF24_STREAM_STOPPED) {
        if (!s->ops.slot31_stop_serialized(s->ops.user)) {
            stream_error(s, WF24_ERR_PLATFORM);
            return false;
        }
    }

    s->state = WF24_STREAM_STOPPED;
    s->current_command = 0u;
    s->last_active_half = 2u;
    return true;
}

bool
wf24_stream_start(wf24_music_stream_t *s,
                  const wf24_music_asset_t *asset)
{
    wf24_slot31_format_t format;

    if (!s || !wf24_music_asset_validate(asset)) {
        if (s)
            stream_error(s, WF24_ERR_BAD_ASSET);
        return false;
    }

    if (s->state != WF24_STREAM_STOPPED && !wf24_stream_stop(s))
        return false;

    s->asset = *asset;
    s->source_sample = 0u;
    s->refills = 0u;
    s->source_exhaustions = 0u;
    s->service_gap_errors = 0u;
    s->current_command = asset->command;
    s->last_active_half = 2u;
    s->eof_seen = 0u;
    s->eof_half = 2u;
    s->error = WF24_ERR_NONE;
    s->state = WF24_STREAM_PRIMING;

    wf24_slot31_exact_format(&format);
    if (!s->ops.slot31_prepare_serialized(&format, s->ops.user)) {
        stream_error(s, WF24_ERR_PLATFORM);
        return false;
    }

    if (!fill_half(s, 0u))
        return false;

    if (s->eof_seen && s->asset.lifetime == WF24_FINITE) {
        /* Never expose stale sound RAM behind a short finite cue. */
        memset(s->staging, 0, sizeof(s->staging));
        if (!s->ops.copy_half_be(1u, s->staging,
                                 WF24_MUSIC_HALF_BYTES, s->ops.user)) {
            stream_error(s, WF24_ERR_PLATFORM);
            return false;
        }
        ++s->refills;
    } else if (!fill_half(s, 1u)) {
        return false;
    }

    if (!s->ops.slot31_start_serialized(s->ops.user)) {
        stream_error(s, WF24_ERR_PLATFORM);
        return false;
    }

    s->state = WF24_STREAM_PLAYING;
    return true;
}

bool
wf24_stream_service(wf24_music_stream_t *s, uint32_t delta_us)
{
    unsigned ca;
    unsigned active;
    unsigned safe;

    if (!s)
        return false;
    if (s->state == WF24_STREAM_STOPPED)
        return true;
    if (s->state != WF24_STREAM_PLAYING)
        return false;

    if (delta_us >= WF24_MUSIC_HALF_US) {
        ++s->service_gap_errors;
        stop_slot_after_playback_error(s);
        stream_error(s, WF24_ERR_SERVICE_GAP);
        return false;
    }

    if (!s->ops.slot31_read_ca_serialized(&ca, s->ops.user)) {
        stop_slot_after_playback_error(s);
        stream_error(s, WF24_ERR_PLATFORM);
        return false;
    }
    if (!wf24_ca_valid(ca)) {
        stop_slot_after_playback_error(s);
        stream_error(s, WF24_ERR_BAD_CA);
        return false;
    }

    active = wf24_active_half(ca);
    if (s->last_active_half == 2u) {
        s->last_active_half = (uint8_t)active;
        return true;
    }

    if (active == s->last_active_half)
        return true;

    if (s->eof_seen &&
        s->asset.lifetime == WF24_FINITE &&
        s->last_active_half == s->eof_half) {
        return wf24_stream_stop(s);
    }

    safe = active ^ 1u;
    if (!(s->eof_seen && s->asset.lifetime == WF24_FINITE)) {
        if (!fill_half(s, safe)) {
            stop_slot_after_playback_error(s);
            return false;
        }
    }

    s->last_active_half = (uint8_t)active;
    return true;
}
