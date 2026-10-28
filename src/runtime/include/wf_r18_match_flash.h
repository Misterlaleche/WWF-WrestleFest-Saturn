#ifndef WF_R18_MATCH_FLASH_H
#define WF_R18_MATCH_FLASH_H
#include "wf_r18_prematch.h"
typedef struct wf_r18_match_flash {
    uint32_t pc,blocked_dependency,owner;
    uint32_t saved_d[8],saved_a[7],hold_d0;
} wf_r18_match_flash_t;
typedef struct wf_r18_match_flash_context {
    wf_r18_prematch_t *machine;
    uint8_t *fg_0c0000;
    size_t fg_bytes;
    wf_r18_prematch_io_fn io;
    void *io_user;
} wf_r18_match_flash_context_t;
/* Original $8F50 or $8FD6 foreground flash. Writes exactly $C0000..$C2FFF
 * as3072 raw LONGs per color; last4KiB FG padding remains untouched.
 * Eight original patterns, three real IRQ ACKs each. $8F50 additionally
 * runs native$21E6 with D0.W=$20:33 physical high/low VBlank periods.
 * SYNC uses original $1C006F.bit1; READ16 uses raw $140026.wordbit10.
 * No wall clock, ACK, palette fade or input is synthesized. WAIT retains
 * the original poll PC when callback supplies no ACK/edge. The IO callback
 * may perform real IRQ side effects on the shared RAM/registers.
 * DONE2 stops before RTS$8FD4/$9050, after full MOVEM restoration and
 * original CLR.L$80. Uses PREMATCH_MORE/WAIT/DONE/BLOCKED/BAD_CONTEXT.
 * Failed dependencies retain executed prefixes, as for prematch. CCR/A7
 * temporary stack are outside this ABI; root owns surrounding IRQ caller.
 */
void wf_r18_match_flash_init(wf_r18_match_flash_t *state,uint32_t owner_rom);
int wf_r18_match_flash_step(wf_r18_match_flash_t *state,
    wf_r18_match_flash_context_t *context);
#endif
