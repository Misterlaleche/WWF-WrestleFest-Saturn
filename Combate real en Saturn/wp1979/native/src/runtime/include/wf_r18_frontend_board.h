#ifndef WF_R18_FRONTEND_BOARD_H
#define WF_R18_FRONTEND_BOARD_H
#include "wf_r18_metasprite.h"

typedef wf_status_t (*wf_r18_frontend_store_bus_fn)(void *,uint32_t,uint32_t,unsigned);
typedef wf_status_t (*wf_r18_frontend_read_bus_fn)(void *,uint32_t,uint32_t *,unsigned);
typedef struct wf_r18_frontend_board {
 uint8_t *ram; /* Borrowed, authoritative big-endian $1C0000..$1C3FFF. */
 size_t ram_bytes;
 wf_r17_m68k_shadow_t *regs;
 uint32_t *a; /* A0..A6, raw32-bit values. */
 wf_r18_read_rom_fn read_rom;
 wf_r18_frontend_store_bus_fn store_bus;
 wf_r18_frontend_read_bus_fn read_bus;
 void *user;
 uint32_t blocked_dependency,blocked_address;
} wf_r18_frontend_board_t;

/* Exact synchronous payloads $B53A/$27012/$27066/$2946A/$264AA/$2006.
 * Bus callbacks receive physical24-bit addresses, big-endian scalar values
 * and widths1/2/4, in original instruction order. They must commit a whole
 * access or fail without mutation, and must not modify this context.
 * No runtime import/export, actor cast, clock, allocation or frame gating.
 * The IRQ caller owns $76/$6F gates and $26E1A/$26DAC map/display owners.
 * $2946A accepts producer queues up to48 bytes each; invalid counts block.
 * Failure may follow already-committed accesses: do not replay a failed call
 * blindly; stop at blocked_dependency (original instruction PC) and inspect
 * blocked_address. Missing callbacks fail closed when actually required.
 * CCR/SR and temporary stack traffic are outside this payload API.
 *
 * Owner$2988 is ONLY its payload $2996..$29F6: call after the driver has
 * observed $140026.bit2 HIGH at$298C; the driver must then observe LOW at
 * $29F6 before advancing its caller. This function does not poll or invent
 * those transitions. It clears RAM$1600..$163F,512 LONG stores to palette
 * payloads (32bytes in each128byte bank; padding untouched), then WORD zero
 * to$184000. D0/A0-A2 restored; D1.W=$FFFF and A3=$180800 stay live exactly
 * as after the original full$2988 return. All other owners preserve D/A.
 */
wf_status_t wf_r18_frontend_board_call(wf_r18_frontend_board_t *,uint32_t owner);
#endif
