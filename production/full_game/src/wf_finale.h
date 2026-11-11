#ifndef WF_FINALE_H
#define WF_FINALE_H
#include "wf_r18_prematch.h"
#include "wf_r18_metasprite.h"

enum {WF_FINALE_MORE=0,WF_FINALE_REQUEST=1,WF_FINALE_BOOT=2,
 WF_FINALE_WAIT=3,WF_FINALE_BLOCKED=-1,WF_FINALE_BAD_CONTEXT=-2};
enum {WF_FINALE_CALL=0,WF_FINALE_READ16=1,WF_FINALE_WRITE16=2,
 WF_FINALE_READ8=4,WF_FINALE_LEVEL=5};
typedef struct {
 uint32_t kind,owner,call_pc,return_pc,address,value;
} wf_finale_request_t;
typedef struct {
 wf_r18_prematch_t *machine;
 wf_r18_read_rom_fn read_rom;void *user;
} wf_finale_context_t;
typedef struct {
 uint32_t pc,blocked_dependency,saved_d[2],saved_a[3];
 wf_finale_request_t request;
 uint8_t pending,fatal,carry;
} wf_finale_t;

/* Original shared TAG/Royal final $1B4E..$1DCE, stopping before JMP $6FC.
 * Raw RAM, D/A registers and ROM tables are borrowed. The nine portraits and
 * six scrolling credits objects retain their original timing and input
 * skips. $250E reads the original pointer table and visits all nine active
 * actors, including untouched slots6..8. Its carry is explicit because this
 * data/address ABI has no CCR. Only D0/D1/A0-A2 are restored by the MOVEM.
 * The final record collection filters ACTIVE.bit7 AND actor+$A1.bit0 and
 * leaves unused LONGs untouched, before mandatory $65C0 and $6FC.
 *
 * CALL dependencies must finish against the same live machine before
 * complete(). READ values are physical, uninverted bus data. LEVEL is the
 * original BYTE $140026.bit2 busy poll; request.value is the required0/1
 * level, and complete() receives the byte actually read. A mismatched level
 * returns WAIT with the request still pending. The adapter must advance the
 * real board only while waiting and reread the byte; it owns IRQ/generation
 * cancellation and A-register synchronization. Repeated pending step() is
 * inert. No clock, input, score, seed, gameplay result or palette is invented.
 * WRITE16 includes the original watchdog clears and priority write. Missing
 * ROM/effects are fatal prefixes and cannot replay. No audio is played here;
 * the original $2052 events remain mandatory calls to the transport.
 */
void wf_finale_init(wf_finale_t *);
int wf_finale_step(wf_finale_t *,wf_finale_context_t *);
int wf_finale_complete(wf_finale_t *,wf_finale_context_t *,wf_status_t,uint16_t);
#endif
