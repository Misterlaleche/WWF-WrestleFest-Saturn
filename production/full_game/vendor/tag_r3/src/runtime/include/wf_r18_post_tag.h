#ifndef WF_R18_POST_TAG_H
#define WF_R18_POST_TAG_H
#include "wf_r18_prematch.h"
#include "wf_r18_metasprite.h"
enum {WF_R18_POST_MORE=0,WF_R18_POST_REQUEST=1,WF_R18_POST_AC0=2,
 WF_R18_POST_RETURN=3,WF_R18_POST_BOUNDARY=4,WF_R18_POST_WAIT=5,
 WF_R18_POST_BLOCKED=-1,WF_R18_POST_BAD_CONTEXT=-2};
enum {WF_R18_POST_CALL=0,WF_R18_POST_READ16=1,WF_R18_POST_WRITE16=2,
 WF_R18_POST_SYNC=3,WF_R18_POST_READ8=4};
typedef struct wf_r18_post_tag_request {
 uint32_t kind,owner,call_pc,return_pc,address,value;
} wf_r18_post_tag_request_t;
typedef struct wf_r18_post_tag_context {
 wf_r18_prematch_t *machine;
 wf_r18_read_rom_fn read_rom;void *user;
} wf_r18_post_tag_context_t;
typedef struct wf_r18_post_tag {
 uint32_t pc,entry_pc,saved_d0,sub_return,blocked_dependency;
 uint32_t saved_d[8],saved_a[4];
 wf_r18_post_tag_request_t request;
 uint8_t pending,fatal,carry;
 uint32_t callback_a0; /* B0C8 MOVEM D0/A0; saved_d0 retains D0. */
} wf_r18_post_tag_t;
/* Raw post-TAG main $11B6/$122C/$19BA and Royal GAME OVER continuation
 * $1B1E: gather active participants, call $65C0 and transfer to $6FC.
 * The Royal result display $1AFC..$1B1D is a separate preceding owner.
 * Through the original next-match
 * JMP $AC0. $11B6 includes the four scratch LONG clears and exact in-ring
 * result waits; $122C is available after an independently completed result.
 * Every original presentation call is mandatory, including B608/BDA6 before
 * progression changes. $6FC and final $1B4E return explicit BOUNDARY PCs.
 * All RAM, D/A and table data are borrowed. No reset, new seed, identity,
 * progression, input, IRQ, time or memory allocation is supplied.
 * CALL's dependency must finish before complete(); READ16 returns physical
 * bus data; WRITE16 writes without changing CPU state; SYNC requests an
 * actual IRQ/ACK for $1C006F.bit1 and returns WAIT while ACK is unchanged.
 * Repeated pending step is inert. Missing effects are fatal, never skipped.
 * Also admits standalone BDA6, AE20(arguments0..3), B0C8, B152, BF5E,
 * C004 and15F4 seams. AE20(0) is the original title-match presenter,
 * including mandatory B29E/B2E4 FG helpers through external CALLs.
 * B608 supplies the complete original milestone4 scene. Continue139C/160E
 * includes original credit checks/spend055A/C404 and physical READ8/READ16.
 * Its206FE/2067C video calls remain explicit dependencies to the existing
 * native referee clear/number helpers. The original AE20 prefix returns
 * immediately for any D0.W!=3 when WORD008C!=0, before MOVEM or writes.
 * With that gate clear, AE20 programs1/2 follow the original attract
 * sequence. Other arguments and final1B4E remain explicit dependencies.
 */
void wf_r18_post_tag_init(wf_r18_post_tag_t *,uint32_t entry_pc);
int wf_r18_post_tag_step(wf_r18_post_tag_t *,wf_r18_post_tag_context_t *);
int wf_r18_post_tag_complete(wf_r18_post_tag_t *,wf_r18_post_tag_context_t *,
 wf_status_t,uint16_t value);
#endif
