#ifndef WF_FAILURE_SCREEN_H
#define WF_FAILURE_SCREEN_H
#include <stdint.h>
#include <stdbool.h>
#include "wf_cd_wait_diagnostics.h"
/* Only the F4C2 dispatcher maintains a current per-actor failure slot. */
static inline unsigned wf_failure_actor_slot(uint32_t owner,unsigned slot){return owner==0xf4c2u&&slot<9?slot:0xffu;}
/* SCU DSTA: levels0..2 operation, standby and interrupted flags.
 * Do not write DnEN or reuse DMA memory while any of these remain set. */
static inline bool wf_failure_dma_idle(const volatile uint32_t *status,unsigned budget){
 while(budget--)if(!(*status&0x00033330u))return true;
 return false;
}
enum {
 WF_FAILURE_ERROR,WF_FAILURE_VIDEO,WF_FAILURE_TILE,WF_FAILURE_ATLAS,
 WF_FAILURE_CD_STAGE,WF_FAILURE_CD_ERROR,WF_FAILURE_IO_STAGE,WF_FAILURE_IO_ERROR,
 WF_FAILURE_FAD,WF_FAILURE_COMMAND,WF_FAILURE_RESPONSE,WF_FAILURE_READ_BYTES,
 WF_FAILURE_COPIED,WF_FAILURE_SESSION,WF_FAILURE_PHASE,WF_FAILURE_PC,
 WF_FAILURE_OWNER,WF_FAILURE_ADDRESS,WF_FAILURE_COMBAT,WF_FAILURE_COMBAT_ERROR,
 WF_FAILURE_DEPENDENCY,WF_FAILURE_CART_ID,WF_FAILURE_CART_BYTES,WF_FAILURE_CART_VALID,
 WF_FAILURE_CODE_COUNT
};
enum {WF_FAILURE_WIDTH=512,WF_FAILURE_HEIGHT=256};
typedef struct {
 uint32_t session_phase,phase,pc,owner,address;
} wf_failure_context_t;
typedef struct {
 uint16_t id,state,animation,x,y,stamina;
 uint8_t active,action;
} wf_failure_actor_t;
typedef struct {
 uint32_t valid,royal,frame,ack,rng,scene,site,render_error,rom_address;
 uint32_t frame_phase,secondary_error,secondary_tile,bad_slot;
 wf_failure_actor_t actors[9];
} wf_failure_combat_t;
/* Optional combat callback; reads live state only on terminal failure. */
void wf_failure_combat_read(wf_failure_combat_t *);
extern const wf_failure_combat_t *wf_failure_combat_state;
const char *wf_failure_actor_name(unsigned);
void wf_failure_combat_bitmap_draw(volatile uint16_t *,const uint32_t[WF_FAILURE_CODE_COUNT],const wf_failure_combat_t *);
/* Optional implementation in each main reads its private current session.
 * Published observer globals may still contain the previous successful step. */
void wf_failure_context_read(wf_failure_context_t *);
uint8_t wf_failure_glyph_column(char,unsigned);
/* Bitmap is 512x256 bytes represented by aligned, big-endian pixel pairs.
 * CPU halfword writes avoid unsupported B-bus byte stores and need no DMA. */
void wf_failure_bitmap_draw(volatile uint16_t *,const uint32_t[WF_FAILURE_CODE_COUNT]);
void wf_failure_screen_show(uint32_t) __attribute__((noreturn));
/* Optional live view; null when the selected reader has no wait telemetry. */
extern const volatile wf_cd_wait_diagnostics_t *wf_failure_cd_wait_state;
extern const uint32_t *wf_failure_codes_ptr;
extern volatile uint32_t wf_failure_shown;
#endif
