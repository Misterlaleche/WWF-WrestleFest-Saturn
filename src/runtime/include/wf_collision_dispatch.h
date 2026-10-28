#ifndef WF_COLLISION_DISPATCH_H
#define WF_COLLISION_DISPATCH_H
#include <stdint.h>
#include "wf_actor.h"
typedef enum { WF_COLL_REJECT=0, WF_COLL_ACCEPT=1, WF_COLL_BLOCKED=2 } wf_coll_result_t;
typedef struct wf_collision_pair_ctx {
 wf_actor_raw_t *src,*dst,*linked;
 uint16_t target_cursor_d1;
 uint8_t source_profile_id;
 uint8_t target_profile_id;
 struct wf_runtime *runtime; /* optional live globals for effects beyond the pair */
} wf_collision_pair_ctx_t;
wf_coll_result_t wf_collision_first_apply(uint8_t selector,wf_collision_pair_ctx_t *c);
wf_coll_result_t wf_collision_second_apply(uint8_t selector,wf_collision_pair_ctx_t *c);
wf_coll_result_t wf_collision_profile_pair_apply(wf_collision_pair_ctx_t *c);
wf_coll_result_t wf_collision_profile_pair_apply_rows(uint8_t source_profile_id,uint8_t target_profile_id,wf_collision_pair_ctx_t *c);
#endif
