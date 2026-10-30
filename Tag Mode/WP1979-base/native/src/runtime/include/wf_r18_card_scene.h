#ifndef WF_R18_CARD_SCENE_H
#define WF_R18_CARD_SCENE_H
#include "wf_r18_prematch.h"
enum {WF_R18_CARD_MORE=0,WF_R18_CARD_REQUEST=1,WF_R18_CARD_RETURN=3,
      WF_R18_CARD_WAIT=5,WF_R18_CARD_BLOCKED=-1,WF_R18_CARD_BAD_CONTEXT=-2};
enum {WF_R18_CARD_CALL=0,WF_R18_CARD_WRITE8=2,WF_R18_CARD_SYNC=3};
typedef struct wf_r18_card_request {
 uint32_t kind,owner,call_pc,return_pc,address,value;
} wf_r18_card_request_t;
typedef struct wf_r18_card_scene {
 uint32_t pc,entry_pc,saved_d0,saved_a0,sub_return,blocked_dependency;
 wf_r18_card_request_t request;
 uint8_t pending,fatal;
} wf_r18_card_scene_t;
/* Original mandatory $7B70 card presentation and local $7F0E/$7E86 owners.
 * Raw 16KiB arcade RAM and live D0-D7/A0-A6. CALL requests expose precise
 * original call/return PCs; caller runs that actual owner and then completes.
 * Flash $8F50/$8FD6 and backend/text/render/world calls remain composition
 * dependencies. WRITE8 commits original board byte $140011=$7B. SYNC polls
 * real IRQ ACK $1C006F.bit1, without advancing a clock or manufacturing ACK.
 * Complete a successful SYNC with ACK still clear to return WAIT at the same
 * poll PC. WF_BLOCKED and other dependency failures are fatal, never waits.
 * Repeated pending REQUEST is inert. Standalone helper entries are supported.
 * RETURN stops before RTS $7E14/$7E84/$8134/$7F0C. CCR/A7 stack are external.
 */
void wf_r18_card_scene_init(wf_r18_card_scene_t *,uint32_t entry_pc);
int wf_r18_card_scene_step(wf_r18_card_scene_t *,wf_r18_prematch_t *);
int wf_r18_card_scene_complete(wf_r18_card_scene_t *,wf_r18_prematch_t *,wf_status_t);
#endif
