#ifndef WF_R17_RAW23_H
#define WF_R17_RAW23_H
#include <stdint.h>
#include "wf_runtime.h"

typedef wf_status_t (*wf_r17_audio_emit_fn)(uint16_t command, void *user);
typedef wf_status_t (*wf_r17_special_215b6_fn)(wf_runtime_t *rt, wf_actor_raw_t *actor, void *user);

typedef struct wf_r17_raw23_ctx {
    uint8_t mode_0161;          /* arcade $1C0161 */
    uint8_t global_15d2;        /* arcade $1C15D2 */
    uint8_t global_15d3;        /* arcade $1C15D3 */
    uint16_t shake_1800;        /* arcade $1C1800 */
    uint16_t referee_1150;      /* arcade $1C1150 */
    uint16_t referee_1180;      /* arcade $1C1180 */
    uint16_t referee_11b0;      /* arcade $1C11B0 */
    uint16_t referee_11e0;      /* arcade $1C11E0 */
    uint8_t referee_1151;       /* arcade $1C1151 */
    uint8_t referee_11b1;       /* arcade $1C11B1 */
    wf_r17_audio_emit_fn audio_emit;
    wf_r17_special_215b6_fn special_215b6;
    void *user;
} wf_r17_raw23_ctx_t;

typedef struct wf_r17_raw23_trace {
    uint32_t callback_rom;
    uint32_t descriptor_rom;
    uint32_t tick_descriptor_rom;
    uint32_t blocked_dependency_rom;
    uint8_t substate;
} wf_r17_raw23_trace_t;

/* Restricted, ROM-exact implementation of the $1C03E state-5 raw-action path
 * for selector $23 only. Other states/selectors remain fail-closed. */
wf_status_t wf_r17_dispatch_1c03e_raw23(wf_runtime_t *rt, unsigned slot,
                                       wf_r17_raw23_ctx_t *ctx,
                                       wf_r17_raw23_trace_t *trace);

typedef struct wf_r17_raw23_bridge {
    wf_r17_raw23_ctx_t *ctx;
    wf_r17_raw23_trace_t trace[WF_ACTOR_SLOTS];
} wf_r17_raw23_bridge_t;

/* Adapter with the $F4C2 dispatcher-hook ABI. It remains deliberately
 * raw-$23-only; non-$23 actors fail closed rather than being approximated. */
wf_status_t wf_r17_dispatch_1c03e_raw23_hook(wf_runtime_t *rt,
                                            wf_actor_raw_t *actor,
                                            void *user);

/* Exposed exact primitives used by raw $23 and regression tests. */
wf_status_t wf_r17_trajectory_26ae(wf_actor_raw_t *actor,
                                   const wf_actor_raw_t *target,
                                   uint16_t index);
int wf_r17_helper_11152_carry(const wf_actor_raw_t *actor,
                              const wf_actor_raw_t *linked);
int wf_r17_helper_11412_carry(const wf_runtime_t *rt,
                              const wf_actor_raw_t *actor);

#endif
