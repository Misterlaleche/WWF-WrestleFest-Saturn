#ifndef WF_R18_FRONTEND_IRQ_H
#define WF_R18_FRONTEND_IRQ_H
#include "wf_r18_title.h"

/* Raw control-plane events. READ16 returns a word in event.value. CALL
 * owns the original helper's complete RAM/D/A/SR effects; return zero on
 * success. WRITE16 commits the supplied word without changing CPU state.
 * No native callback may silently omit a required dependency. */
enum { WF_R18_CONTROL_CALL=0, WF_R18_CONTROL_WRITE16=1,
       WF_R18_CONTROL_READ16=2 };
typedef struct wf_r18_frontend_control_context wf_r18_frontend_control_context_t;
typedef int (*wf_r18_frontend_control_effect_fn)(void *user,
    wf_r18_title_event_t *event,wf_r18_frontend_control_context_t *context);
struct wf_r18_frontend_control_context {
    uint8_t *ram_1c0000;
    size_t ram_bytes;
    wf_r17_m68k_shadow_t *regs;
    uint32_t *a; /* A0..A6 */
    uint32_t ssp,usp; /* Quiescent stack pointers; temporary BSR/MOVEM omitted. */
    uint16_t sr; /* Real 68000 SR, including S and CCR. */
    wf_r18_frontend_control_effect_fn effect;
    void *user;
};
typedef struct wf_r18_frontend_irq {
    uint32_t pc,blocked_dependency;
    uint32_t saved_d[8],saved_a[7],return_pc;
    uint16_t return_sr;
    uint8_t frame_valid,registers_saved;
} wf_r18_frontend_irq_t;
enum { WF_R18_CONTROL_MORE=0, WF_R18_CONTROL_WAIT=1, WF_R18_CONTROL_HANDOFF=2,
       WF_R18_CONTROL_RTE=3,
       WF_R18_CONTROL_BLOCKED=-1, WF_R18_CONTROL_BAD_CONTEXT=-2 };

/* Accepted-credit diversion $8AE..$8F2 only. Caller has already run $978
 * and enters in supervisor mode. Sets SSP=$1C3BD4, USP=$1C3FD4, clears
 * WORD$76 and IRQ port$140000, updates $6F, restores masked SR in user
 * mode, invokes $1F6C/$1FDE/$1F9E and stops before JMP target$52BE.
 * Global VBlank/RTE, $76/$7C gates and credit acceptance remain caller-owned.
 * Stack scratch $1C3AD4..$1C3FFF is not materialized in the RAM buffer;
 * all persistent RAM below $1C3AD4 and quiescent SSP/USP/SR are exact.
 * A failed callback blocks at its original PC; it must not mutate on failure.
 */
void wf_r18_frontend_irq_init(wf_r18_frontend_irq_t *state);
int wf_r18_frontend_irq_step(wf_r18_frontend_irq_t *state,
    wf_r18_frontend_control_context_t *context);
/* Whole IRQ $834..$918. The platform has already made a 6-byte exception
 * frame: context.ssp points at it, context.sr is the supervisor IRQ SR.
 * interrupted_sr/pc are its explicit contents. READ16$140020.bit9 is the
 * original busy gate; no pulse or frame is synthesized. CALL dependencies
 * are $B4B8,$B53A,$27012,$27066,$26E1A,$2946A,$26DAC,$264AA,$978,$1E92.
 * $978 must return the original carry in context.sr. ACK and reentry gates
 * follow original RAM$76/$6F/$7C. Normal RTE restores D/A, loads frame SR,
 * adds six to SSP, sets state.pc=interrupted_pc and returns CONTROL_RTE.
 * Accepted START abandons the saved frame/registers as in $8AE and instead
 * returns HANDOFF$52BE. init() retains the direct $8AE seam above.
 */
void wf_r18_frontend_irq_enter(wf_r18_frontend_irq_t *state,
    uint16_t interrupted_sr,uint32_t interrupted_pc);
#endif
