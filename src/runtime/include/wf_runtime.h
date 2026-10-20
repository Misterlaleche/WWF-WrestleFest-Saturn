#ifndef WF_RUNTIME_H
#define WF_RUNTIME_H
#include <stdint.h>
#include <stddef.h>
#include "wf_actor.h"
#include "wf_r17_m68k_shadow.h"

typedef enum wf_status { WF_OK=0, WF_BLOCKED=1, WF_BAD_STATE=2 } wf_status_t;
typedef struct wf_runtime wf_runtime_t;
typedef wf_status_t (*wf_phase_fn)(wf_runtime_t *rt, uint32_t rom_addr, void *user);

typedef struct wf_phase_binding { uint32_t rom_addr; wf_phase_fn fn; void *user; } wf_phase_binding_t;

typedef struct wf_globals_raw {
 /* Direct arcade globals retained by numeric address role where gameplay consumes them. */
 uint8_t credit_cycle_units_004b; /* $1C004B */
 uint8_t credit_remainder_004e;   /* $1C004E */
 uint8_t credit_count_004f;       /* $1C004F */
 uint16_t config_0066;            /* $1C0066/$0067 */
 uint8_t frame_handshake[2]; /* $1C006E/$6F equivalent */
 uint8_t gameplay_vblank_enable; /* $1C0076 */
 uint8_t global_0063; /* $1C0063 BCD credit display byte */
 uint16_t global_0072; /* $1C0072 credit display cache, initialized by $8AE6 */
 uint16_t credited_route; /* $1C007C */
 uint32_t global_0092; /* $1C0092 selected Royal actor pointer */
 uint16_t global_015e, global_0164; /* raw status-table aliases */
 uint32_t frame_counter; /* $1C0080 */
 uint8_t global_0082; /* $1C0082 high byte; with pipeline_mode forms WORD $1C0082 */
 uint8_t pipeline_mode; /* $1C0083; bit0 selects A/B */
 uint8_t mode_0160; /* $1C0160 */
 uint8_t mode_0161; /* $1C0161 */
 uint16_t progression_0162; /* $1C0162: CPU progression selector used by state6/controller15 */
 uint16_t global_0156[4]; /* $1C0156/$158/$15A/$15C Royal status words */
 uint16_t global_0166; /* $1C0166 word */
 uint16_t global_1682; /* $1C1682 referee countdown */
 uint32_t global_1684; /* $1C1684 referee actor pointer */
 uint32_t global_1688; /* $1C1688 referee actor pointer */
 uint32_t global_168c; /* $1C168C exact target candidate pointer */
 uint16_t global_1692; /* $1C1692 target metric */
 uint16_t global_1690; /* $1C1690/$1691 TAG participant owner */
 uint16_t global_1698; /* $1C1698 TAG delayed-selection timer */
 uint16_t global_1694; /* $1C1694 Royal replacement counter */
 uint8_t controller_suppress;
 uint8_t global_1697; /* $1C1697, only numeric bit contract is used */
 uint16_t global_169a; /* $1C169A count display accumulator word */
 uint8_t global_169b; /* $1C169B */
 uint16_t global_169e; /* $1C169E */
 uint16_t global_16a0; /* $1C16A0 BCD minutes */
 uint16_t global_16a2; /* $1C16A2 BCD seconds */
 uint16_t global_16a4; /* $1C16A4 timer subcounter */
 uint8_t global_16a7; /* $1C16A7 Royal pending replacement count */
 uint16_t royal_occupancy[12]; /* $1C16A8..$1C16BF, lower 0..5 + upper 6..11 */
 uint16_t global_16c4; /* $1C16C4: high byte $16C4, low byte $16C5 */
 uint16_t global_16ca; /* $1C16CA word */
 uint16_t global_16cc; /* $1C16CC: high-byte flags, low-byte $16CD countdown */
 uint16_t global_16ce; /* $1C16CE: low byte $16CF rotating slot cache */
 uint16_t global_15ce; /* $1C15CE Royal reserve/full marker used by $18C4 */
 uint8_t global_1678; /* $1C1678 transition owner flags */
 uint8_t global_167a; /* $1C167A raw message/status producer state */
 uint8_t global_167b; /* $1C167B previous/cache state */
 uint32_t global_1670; /* $1C1670 */
 uint32_t global_1674; /* $1C1674 */
 uint16_t global_007e; /* $1C007E */
 uint8_t global_007f;  /* $1C007F */
 /* Branch-B raw map/scroll state. Numeric names preserve ROM ownership. */
 uint16_t global_16d2; /* $1C16D2 */
 uint16_t global_16d4; /* $1C16D4 */
 uint16_t global_17d6; /* $1C17D6 */
 uint16_t global_17d8; /* $1C17D8 */
 uint16_t global_17da; /* $1C17DA */
 uint16_t global_17dc; /* $1C17DC */
 uint16_t global_17de; /* $1C17DE */
 uint16_t global_17e0; /* $1C17E0 */
 uint16_t global_17e2; /* $1C17E2 */
 uint16_t global_17e4; /* $1C17E4 */
 uint16_t global_17e6; /* $1C17E6 */
 uint16_t global_17ea; /* $1C17EA */
 uint16_t global_17ee; /* $1C17EE */
 uint16_t global_17f2_word; /* $1C17F2 exact WORD */
 uint16_t global_17f6; /* $1C17F6 */
 uint16_t global_17fa; /* $1C17FA */
 uint16_t global_17fe; /* $1C17FE */
 uint16_t global_1800; /* $1C1800 */
 uint16_t global_1150; /* $1C1150 */
 uint16_t global_1180; /* $1C1180 */
 uint16_t global_11b0; /* $1C11B0 */
 uint16_t global_11e0; /* $1C11E0 */
 uint16_t global_1804; /* $1C1804 */
 uint16_t global_1806; /* $1C1806 */
 uint8_t transfer_queue_1808[16]; /* $1C1808-$1C1817 */
 uint16_t transfer_count_1828; /* $1C1828 */
 uint16_t global_15f4; /* $1C15F4 */
 uint16_t global_15fc; /* $1C15FC scene palette selector */
 uint16_t global_15f8; /* $1C15F8 */
 uint8_t global_15d2; /* high byte WORD $1C15D2 */
 uint8_t global_15d3; /* low byte WORD $1C15D2 */
 uint8_t global_15d4; /* high byte WORD $1C15D4 */
 uint8_t global_15d5; /* low byte WORD $1C15D4 */
 uint16_t global_15d6; /* $1C15D6 */
 uint16_t global_15d8; /* $1C15D8 */
 uint16_t global_00aa; /* $1C00AA word */
 uint16_t global_00b2; /* $1C00B2 word; low byte at $1C00B3 */
 uint8_t global_00a3; /* absolute $00A3 raw gameplay byte */
 uint8_t cabinet_edge_15da;  /* $1C15DA */
 uint8_t cabinet_level_15db; /* $1C15DB */
 uint8_t cabinet_edge_15dc;  /* $1C15DC */
 uint8_t cabinet_level_15dd; /* $1C15DD */
 uint8_t global_16e6; /* $1C16E6, $8710 helper $8856 probe */
 uint16_t royal_slots_1586[10]; /* $1C1586, 10-word Royal scheduler table */
 /* Selector bytes whose addresses are not already represented above.
  * Accessors must redirect $1586..$1599 and $15CE..$15CF to their named
  * fields; these overlapping bytes are not a second authoritative copy. */
 uint8_t royal_selector_1576[0x5c]; /* $1C1576..$1C15D1 */
 uint8_t cabinet_tag_15e0[4]; /* $1C15E0..$1C15E3 */
 uint32_t rng_state;  /* arcade $1C005C equivalent: value must be externally sourced */
 uint32_t rng_d2, rng_d3, rng_d4, rng_d5, rng_d6, rng_d7; /* caller register context for $21B4 */
 uint8_t rng_state_valid;
 uint8_t rng_register_context_valid;
} wf_globals_raw_t;

struct wf_runtime {
 wf_globals_raw_t g;
 wf_actor_raw_t actors[WF_ACTOR_SLOTS];
 wf_actor_raw_t fixed_object0; /* arcade $1C0F1C */
 wf_actor_raw_t fixed_object1; /* arcade $1C1028 */
 wf_actor_raw_t fixed_four_1134; /* arcade $1C1134, $10120 object 0 */
 wf_actor_raw_t referee_aux;     /* arcade $1C1164, $10120 object 1 / $F8E4 */
 wf_actor_raw_t fixed_four_1194; /* arcade $1C1194, $10120 object 2 */
 wf_actor_raw_t fixed_four_11c4; /* arcade $1C11C4, $10120 object 3 */
 wf_actor_raw_t referee;       /* arcade $1C11F4 */
 uint8_t transient_1258[12][0x2a]; /* arcade $1C1258.., 12 records stride $2A */
 /* Branch-B exact raw work areas. */
 uint8_t branchb_list_182a[48]; /* $1C182A..$1C1859 */
 uint8_t branchb_prev_185a[48]; /* $1C185A..$1C1889 */
 uint8_t branchb_objs_188a[48][4]; /* $1C188A, 48 records stride 4 */
 wf_actor_raw_t effect_1450[3]; /* arcade $1C1450, 3 compact records stride $2A; common prefix only */
 wf_actor_raw_t compact_14ce[4]; /* arcade $1C14CE, four compact records stride $2A; common prefix used */
 wf_r17_m68k_shadow_t m68k_regs;
 wf_phase_binding_t *bindings;
 size_t binding_count;
 uint32_t trace[64];
 size_t trace_count;
 uint8_t frame_ready;
 uint8_t vblank_ack;
 uint8_t frame_stage; /* 0=$F9A, 1=wait ACK, 2=post-ACK, 3=direct boot at $FA4 */
};

void wf_runtime_init(wf_runtime_t *rt, wf_phase_binding_t *bindings, size_t binding_count);
wf_status_t wf_runtime_frame(wf_runtime_t *rt);
void wf_runtime_vblank_ack(wf_runtime_t *rt);
wf_status_t wf_dispatch_phase(wf_runtime_t *rt,uint32_t addr);
void wf_for_each_active(wf_runtime_t *rt, void (*fn)(wf_runtime_t*,unsigned,wf_actor_raw_t*,void*), void *user);
wf_actor_raw_t *wf_actor_from_rom_ptr(wf_runtime_t *rt,uint32_t ptr,unsigned *slot_out);
const wf_actor_raw_t *wf_actor_from_rom_ptr_const(const wf_runtime_t *rt,uint32_t ptr,unsigned *slot_out);
void wf_apply_pending_damage_and_tier(wf_actor_raw_t *a);
#endif
