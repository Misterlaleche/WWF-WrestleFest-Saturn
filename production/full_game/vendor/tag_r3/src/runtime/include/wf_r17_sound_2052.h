#ifndef WF_R17_SOUND_2052_H
#define WF_R17_SOUND_2052_H
#include "wf_r17_actions_r11.h"

/* Original $2052 output gate. The transport is already supplied by the
 * gameplay binding through raw23_ctx for every $1C03E invocation.
 * No global sink, new runtime fields, register changes or ROM patches. */
static inline wf_status_t wf_r17_sound_2052(wf_runtime_t *rt,
                                           wf_r17_actions_r11_bridge_t *bridge,
                                           uint16_t command)
{
    if (!rt) return WF_BAD_STATE;
    if (!rt->g.credited_route && (rt->g.config_0066 & 0x20u)) return WF_OK;
    if (!bridge || !bridge->raw23_ctx || !bridge->raw23_ctx->audio_emit)
        return WF_BLOCKED;
    return bridge->raw23_ctx->audio_emit((uint16_t)(command | 0x3100u),
                                         bridge->raw23_ctx->user);
}

/* Original $1110E: actor FLAGS33.bit2 chooses the ringside impact cue. */
static inline wf_status_t wf_r17_sound_1110e(wf_runtime_t *rt,
                                            const wf_actor_raw_t *actor,
                                            wf_r17_actions_r11_bridge_t *bridge)
{
    if (!actor) return WF_BAD_STATE;
    return wf_r17_sound_2052(rt, bridge,
                             (wf_u8(actor, 0x33u) & 4u) ? 0x2du : 0x29u);
}

/* Original $1112E: running footstep on poses BYTE1/4 when TIMER.W is zero. */
static inline wf_status_t wf_r17_sound_1112e(wf_runtime_t *rt,
                                            const wf_actor_raw_t *actor,
                                            wf_r17_actions_r11_bridge_t *bridge)
{
    if (!actor) return WF_BAD_STATE;
    unsigned pose = wf_u8(actor, 0x25u);
    if (wf_be16(actor, 0x22u) || (pose != 1u && pose != 4u)) return WF_OK;
    return wf_r17_sound_2052(rt, bridge, 0x33u);
}
#endif
