#ifndef WF_R18_FRONTEND_COIN_H
#define WF_R18_FRONTEND_COIN_H
#include "wf_r18_frontend_irq.h"
typedef struct wf_r18_frontend_coin {
    uint32_t pc,blocked_dependency,saved_d[3],return_pc;
    uint16_t return_sr;
    uint8_t frame_valid,registers_saved;
} wf_r18_frontend_coin_t;
/* IRQ vector$434..$48A. Platform supplies the exception frame explicitly,
 * as for frontend_irq_enter. Raw READ16$140020 provides high-byte bits0/1
 * (word$0100/$0200) for coin pulses and bit2 (word$0400) for service.
 * WRITE16 acknowledges$140016 and
 * $140002 and sends original command$312A to$14000C on a counted coin.
 * $48C qualifies releases with >=4 inactive polls and duration<0x360;
 * $4BC credits service after >0x1E active polls. $4F2 uses original ratios
 * $4B/$4D and caps normal credits at99; service increment is an uncapped
 * byte addition. No coin is invented or debounced by elapsed wall time.
 * Pure helpers use live D0/D1/D2 exactly and preserve all other registers.
 * Missing callbacks block; helpers restore owned RAM/D/SR on failure.
 * Temporary stack memory above$1C3AD4 is omitted; SSP/USP/SR are exact at
 * public boundaries. RTE restores D0-D2 and returns CONTROL_RTE.
 */
void wf_r18_frontend_coin_enter(wf_r18_frontend_coin_t *state,
    uint16_t interrupted_sr,uint32_t interrupted_pc);
int wf_r18_frontend_coin_step(wf_r18_frontend_coin_t *state,
    wf_r18_frontend_control_context_t *context);
int wf_r18_frontend_coin_pulse_048c(wf_r18_frontend_control_context_t *context);
int wf_r18_frontend_coin_service_04bc(wf_r18_frontend_control_context_t *context);
int wf_r18_frontend_coin_credit_04f2(wf_r18_frontend_control_context_t *context);
#endif
