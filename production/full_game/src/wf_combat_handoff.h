#ifndef WF_COMBAT_HANDOFF_H
#define WF_COMBAT_HANDOFF_H
#include "wf_session.h"
#include "wf_board.h"
#include "wf_pacer.h"
/* Same-build native capsule, not an on-disc save format. No pointer may be
 * consumed before wf_combat_join() has rebound the new executable's owners. */
typedef struct {
 uint32_t a[7],ssp,usp,pc,generation;
 uint16_t sr;
 uint8_t phase,credited_start,world_visible,reserved;
} wf_frontend_checkpoint_t;
typedef struct {
 uint32_t magic,version,bytes,checksum;
 uint8_t ram[0x4000];
 wf_runtime_t runtime;
 wf_r18_video_t video;
 wf_board_t board;
 wf_pacer_t pacer;
 uint32_t fields,clock_origin,cold_rng,direction;
 wf_frontend_checkpoint_t frontend;
} wf_combat_handoff_t;
bool wf_combat_handoff_pack(wf_combat_handoff_t *,const wf_session_t *,const wf_board_t *,const wf_pacer_t *);
bool wf_combat_handoff_restore(const wf_combat_handoff_t *,wf_session_t *,wf_board_t *,wf_pacer_t *);
/* Reverse transfer requires a naturally completed, acknowledged combat and
 * captured world/records. The new frontend then runs the deferred original
 * scene_resume export. Neither direction runs frontend RESET. */
bool wf_combat_handoff_pack_return(wf_combat_handoff_t *,const wf_session_t *,const wf_board_t *,const wf_pacer_t *);
bool wf_combat_handoff_restore_return(const wf_combat_handoff_t *,wf_session_t *,wf_board_t *,wf_pacer_t *);
void wf_combat_handoff_seal(wf_combat_handoff_t *);
#endif
