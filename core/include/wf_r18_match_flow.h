#ifndef WF_R18_MATCH_FLOW_H
#define WF_R18_MATCH_FLOW_H
#include "wf_r18_match_setup.h"
enum {WF_R18_MATCH_MORE=0,WF_R18_MATCH_REQUEST=1,WF_R18_MATCH_GAMEPLAY=2,
      WF_R18_MATCH_RETURN=3,WF_R18_MATCH_BOUNDARY=4,
      WF_R18_MATCH_BLOCKED=-1,WF_R18_MATCH_BAD_CONTEXT=-2};
enum {WF_R18_MATCH_CALL=0,WF_R18_MATCH_ENTRY=1};
typedef struct wf_r18_match_request {uint32_t kind,owner,call_pc,return_pc;} wf_r18_match_request_t;
typedef struct wf_r18_match_flow {
 uint32_t pc,entry_pc,saved_d0,blocked_dependency;
 wf_r18_match_request_t request;
 uint8_t pending,fatal;
} wf_r18_match_flow_t;
/* Raw paid controller $AC0/$BD2..$C96, common finalize $8AE6..$8B38 and
 * original JMP $F8C. Also supports standalone BD2/C6C/8AE6 return boundaries.
 * CALL requests preserve the exact live context and known original return PC.
 * Caller composes the actual owner cursor (e.g. C98, A654, fades, 7B70) and
 * calls complete only after that owner finishes. WF_BLOCKED remains fatal;
 * it is never treated as an asynchronous wait. Repeated REQUEST is inert.
 * $7B70 is mandatory at $C02 even on the first match; it is a separate
 * presentation owner, not roster $57E0. Attract $B1C is an explicit dependency.
 * Entry gates $6FC/$57E0 request their original owner without executing it.
 * Complete ENTRY with its actual reported continuation PC; BOUNDARY returns
 * that PC for the outer driver to dispatch/reinitialize, without inference.
 * No RT import, main reset, chosen identity, RNG seed, input or clock supplied.
 */
void wf_r18_match_flow_init(wf_r18_match_flow_t *,uint32_t entry_pc);
int wf_r18_match_flow_step(wf_r18_match_flow_t *,wf_r18_prematch_t *);
/* resume_pc=0 selects a CALL's fixed return. ENTRY requires actual nonzero PC. */
int wf_r18_match_flow_complete(wf_r18_match_flow_t *,wf_r18_prematch_t *,
                              wf_status_t status,uint32_t resume_pc);
#endif
