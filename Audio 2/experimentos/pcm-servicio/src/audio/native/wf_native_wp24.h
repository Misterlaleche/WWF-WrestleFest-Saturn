#ifndef WF_NATIVE_WP24_H
#define WF_NATIVE_WP24_H
#include "wf_native_host.h"
#include "../wp24/wf24_music_stream.h"
typedef struct {
    wf_native_host_t *host;
    wf24_stream_ops_t source;
} wf_native_wp24_t;
/* Retain R04's read/render producers. Bind slot controls to the 68000 owner.
 * Output ops.user points to bridge, which must outlive the stream. */
bool wf_native_wp24_bind(wf_native_wp24_t *bridge,wf_native_host_t *host,
                         const wf24_stream_ops_t *source,wf24_stream_ops_t *out);
#endif
