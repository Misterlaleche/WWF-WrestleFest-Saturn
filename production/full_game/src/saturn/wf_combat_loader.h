#ifndef WF_COMBAT_LOADER_H
#define WF_COMBAT_LOADER_H
#include "wf_combat_handoff.h"
#define WF_COMBAT_CAPSULE ((wf_combat_handoff_t*)0x20201000u)
#define WF_COMBAT_MAILBOX ((volatile uint32_t*)0x260fffe0u)
#define WF_COMBAT_MODULE_STAGE 0x20240000u
#define WF_COMBAT_MODULE_LIMIT 0x000c0000u
#define WF_COMBAT_FATAL_MARKER 0xdead0003u
#define WF_FRONTEND_FATAL_MARKER 0xdead0006u
/* 0 frontend;1 loading;2 replacing;3 combat;4 return load;5 return jump;
 * 6 postmatch frontend;0xdead0003/6 verified executable fatal diagnostics.
 * Other0xdead.... values are loader failures, not completed CRT handshakes.
 * Baselines reserve the last4KiB. The alpha validates every allocation below
 * 0x060fffc0 and guards the SDK allocator; the mailbox remains outside it. */
bool wf_combat_load_module(wf_session_t *,wf_board_t *,uint32_t cold_rng);
bool wf_combat_return_frontend(wf_session_t *,wf_board_t *,uint32_t cold_rng);
#endif
