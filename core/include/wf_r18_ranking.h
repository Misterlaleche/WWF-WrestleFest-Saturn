#ifndef WF_R18_RANKING_H
#define WF_R18_RANKING_H
#include "wf_r18_prematch.h"
#include "wf_r18_metasprite.h"

enum {WF_R18_RANK_MORE=0,WF_R18_RANK_REQUEST=1,WF_R18_RANK_RETURN=2,
 WF_R18_RANK_WAIT=3,WF_R18_RANK_BLOCKED=-1,WF_R18_RANK_BAD_CONTEXT=-2};
enum {WF_R18_RANK_CALL=0,WF_R18_RANK_READ=1,WF_R18_RANK_WRITE=2,
 WF_R18_RANK_SYNC=3};
typedef struct wf_r18_ranking_request {
 uint32_t kind,owner,call_pc,return_pc,address,value,width;
} wf_r18_ranking_request_t;
typedef struct wf_r18_ranking_context {
 wf_r18_prematch_t *machine; /* Borrowed full16KB rawRAM/D0-D7/A0-A6. */
 wf_r18_read_rom_fn read_rom;void *user;
} wf_r18_ranking_context_t;
typedef struct wf_r18_ranking {
 uint32_t pc,entry_pc,blocked_dependency,return_stack[8],saved_d[5],saved_a2;
 uint32_t saved_d0,io_value;
 wf_r18_ranking_request_t request;
 uint8_t depth,pending,fatal,carry;
} wf_r18_ranking_t;
/* Original65C0 qualification/records/initials scene, plus its local helper
 * entry points. No RAM clear/default/seed/input/frame clock is synthesized.
 * CALL must execute the requested original owner on the same raw context.
 * READ/WRITE supplies literal bus width1/2/4; SYNC requests a real IRQ ACK
 * for RAM006F.bit1 and returns WAIT if the ACK remains unchanged. Repeated
 * pending step is inert. Unknown/missing effects are fatal, never omitted.
 * Original external video calls include69C8's world3 setup and fades, and
 * A4D4/7874 record graphics and6CFA glyphs. Raw workbuffers1CD4..2787
 * belong to this scene. Local public entries are65C0,6794,67B2,683E,68AC,
 * 68F6,69C8,6A26,6B6E,6CE8 and7720. D/A are instruction-live; CCR/stack
 * are outside this ABI. read_rom may be absent only while no ROM read is
 * reached. Missing reads/effects retain the executed prefix and are fatal.
 */
void wf_r18_ranking_init(wf_r18_ranking_t *,uint32_t entry_pc);
int wf_r18_ranking_step(wf_r18_ranking_t *,wf_r18_ranking_context_t *);
int wf_r18_ranking_complete(wf_r18_ranking_t *,wf_r18_ranking_context_t *,
 wf_status_t,uint32_t value);
#endif
