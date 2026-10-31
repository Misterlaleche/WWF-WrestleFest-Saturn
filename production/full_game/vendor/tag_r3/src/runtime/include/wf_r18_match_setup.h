#ifndef WF_R18_MATCH_SETUP_H
#define WF_R18_MATCH_SETUP_H
#include "wf_r18_prematch.h"
/* Original $1086C setup dispatcher with the raw $1034A TAG branch and existing
 * Royal module. RAM history $0598, roster $0380, replay WORD$008C, all actors,
 * RNG/clock and live D/A are authoritative inputs. Only $2AEA is delegated.
 * D0-D3/A0-A2 use the original MOVEM preservation. No direct-boot substitute.
 * Dependency failures retain executed prefixes; stop or restore before retry. */
wf_status_t wf_r18_match_setup_1086c(wf_r18_prematch_t *);
wf_status_t wf_r18_match_setup_d1c(wf_r18_prematch_t *);
wf_status_t wf_r18_match_setup_ffd2(wf_r18_prematch_t *);
wf_status_t wf_r18_match_setup_1004a(wf_r18_prematch_t *);
typedef struct wf_r18_match_join {uint32_t pc,blocked_dependency;} wf_r18_match_join_t;
/* Full $C98..$D10 common join, returning to caller $C66 before A654.
 * Additional exact external owners: $2988,$1FC0,$1F6C,$26E66,$2A06,$1F9E.
 * They own their original graphics/palette/VBlank operations. This cursor
 * supplies no clock/input/IRQ and retains fatal dependency prefixes.
 * Uses WF_R18_PREMATCH_MORE/DONE/BLOCKED/BAD_CONTEXT status values. */
void wf_r18_match_join_init(wf_r18_match_join_t *);
int wf_r18_match_join_step(wf_r18_match_join_t *,wf_r18_prematch_t *);
#endif
