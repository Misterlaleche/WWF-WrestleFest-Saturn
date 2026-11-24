#ifndef WF_R18_FRONTEND_WORLD_H
#define WF_R18_FRONTEND_WORLD_H
#include "wf_r18_metasprite.h"
typedef struct wf_r18_frontend_world {
 uint8_t *ram;size_t ram_bytes; /* authoritative big-endian16KB */
 wf_r17_m68k_shadow_t *regs;uint32_t *a; /* A0..A6 raw32bits */
 wf_r18_read_rom_fn read_rom;void *user;
 uint32_t blocked_dependency,blocked_address;
} wf_r18_frontend_world_t;
/* Raw285DA list/object/animation owner and standalone28608/2884E/28808/
 * 28896/288F0/28B3E/28B46 helpers. Literal ROM tables, no gameplay import.
 * Current-list tail after FF is retained, previous-list copy is48 bytes;
 * 28896 visits47 objects and28B46 peer scan visits46, in original order.
 * 28608 leaves D6.B as the last source ID. Timer0 underflows toFF before
 * advancing; queued frames go directly to RAM194A/197A,counts19AC/19AA.
 * Handler dispatch admits only exact28B3E (all original world0..6 tables).
 * Unknown handler or out-of-allocation list/object/queue blocks explicitly;
 * executed prefixes remain visible. No rollback/replay transaction, clock,
 * VRAM transport, independent RNG or resource allocation. Board2946A owns
 * queue transport. Standalone288F0 leaves D1.W=id*4 and A1=handler; MOVEM
 * boundaries of other helpers preserve their original D/A register sets.
 */
wf_status_t wf_r18_frontend_world_call(wf_r18_frontend_world_t *,uint32_t owner);
#endif
