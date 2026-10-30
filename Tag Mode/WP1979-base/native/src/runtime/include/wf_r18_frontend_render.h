#ifndef WF_R18_FRONTEND_RENDER_H
#define WF_R18_FRONTEND_RENDER_H
#include "wf_runtime.h"
typedef struct wf_r18_frontend_render wf_r18_frontend_render_t;
typedef wf_status_t (*wf_r18_frontend_emit_fn)(void *,wf_r18_frontend_render_t *);
typedef wf_status_t (*wf_r18_frontend_sprite_clear_fn)(void *,uint32_t);
struct wf_r18_frontend_render {
 uint8_t *ram;wf_r17_m68k_shadow_t *regs;uint32_t *a;
 /* Exact $D1FC owner. A0 selects a raw object (including compact title
  * records); A2 is the current $C2000 sprite cursor and advances on emit.
  * The caller preserves D0-D5/A0-A5. The callback must retain original
  * live D6/D7, A2, A6 and hardware sprite fields; reserved bytes may use
  * the existing normalized graphics representation. */
 wf_r18_frontend_emit_fn emit_d1fc;
 wf_r18_frontend_sprite_clear_fn clear_attribute;
 void *user;uint32_t blocked_dependency;
};
/* $247C screen projection, $27B8 six raw queues and $2836 stable ordering,
 * composition-link consumption and previous-frame tail clear. No actor cast
 * or platform clock; callers supply the exact leaf renderer above. */
wf_status_t wf_r18_frontend_render_call(wf_r18_frontend_render_t *,uint32_t);
#endif
