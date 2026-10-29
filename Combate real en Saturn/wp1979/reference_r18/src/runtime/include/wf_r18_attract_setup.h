#ifndef WF_R18_ATTRACT_SETUP_H
#define WF_R18_ATTRACT_SETUP_H
#include "wf_r18_prematch.h"
enum {WF_R18_ATTRACT_MORE=0,WF_R18_ATTRACT_REQUEST=1,WF_R18_ATTRACT_RETURN=3,
      WF_R18_ATTRACT_WAIT=5,WF_R18_ATTRACT_BLOCKED=-1,WF_R18_ATTRACT_BAD_CONTEXT=-2};
enum {WF_R18_ATTRACT_CALL=0,WF_R18_ATTRACT_READ16=1};
typedef struct wf_r18_attract_request {uint32_t kind,owner,call_pc,return_pc,address,value;} wf_r18_attract_request_t;
typedef struct wf_r18_attract_setup {
 uint32_t pc,entry_pc,saved_d0,sub_return,blocked_dependency;
 wf_r18_attract_request_t request;
 uint8_t pending,fatal;
} wf_r18_attract_setup_t;
/* Original uncredited $B1C..$BD0 and raw $E02..$F20 demo setup, with native
 * $21E6 physical VBlank delay. Entry may be B1C, E02 or21E6. No seed/reset
 * or fixed identity is supplied: the original selector/table/live RAM own it.
 * CALL requires the actual complete owner, then complete(WF_OK,0). Required
 * special branches retain ranking69C8 and visual8490 as explicit dependencies.
 * READ16 returns the raw140026 word to complete; wrong VBlank level returns
 * WAIT at the same poll PC. The21E6 DBRA counts D0.W+1 periods and restores
 * the full saved D0. No manufactured edge or elapsed-time delay is used.
 * Pending requests are inert; failed dependencies are fatal preserved prefixes.
 * RETURN stops before original RTS BD0/F20/2206. CCR/A7 temporary stack external.
 */
void wf_r18_attract_setup_init(wf_r18_attract_setup_t *,uint32_t entry_pc);
int wf_r18_attract_setup_step(wf_r18_attract_setup_t *,wf_r18_prematch_t *);
int wf_r18_attract_setup_complete(wf_r18_attract_setup_t *,wf_r18_prematch_t *,wf_status_t,uint16_t read_word);
#endif
