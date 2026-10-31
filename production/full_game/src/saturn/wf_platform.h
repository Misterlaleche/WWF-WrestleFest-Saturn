#ifndef WF_PLATFORM_H
#define WF_PLATFORM_H
#include <yaul.h>
#include "wf_arcade_clock.h"
/* Sole owner of VBlank/SMPC and logical pacing. FRT/CD/VDP1 handoff pending. */
extern volatile uint32_t wf_fields,wf_error,wf_cart_id,wf_cart_bytes,wf_cart_valid;
extern volatile uint32_t wf_logic_fields,wf_logic_ticks,wf_logic_pending;
void wf_platform_init(void);
void wf_platform_scene_begin(uint32_t phase);
bool wf_platform_tick_wait(void);
bool wf_platform_load(bool begin);
void wf_platform_hold_fields(uint32_t count);
void wf_platform_pad(smpc_peripheral_digital_t *pad);
void wf_platform_fail(uint32_t error);
#endif
