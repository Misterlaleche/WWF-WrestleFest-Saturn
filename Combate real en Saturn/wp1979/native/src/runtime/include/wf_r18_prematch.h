#ifndef WF_R18_PREMATCH_H
#define WF_R18_PREMATCH_H
#include "wf_runtime.h"

/* Raw big-endian RAM $1C0000..$1C3FFF, live data registers and A0..A6.
 * Dependencies run synchronously with this same context. They must implement
 * the named original owner or return WF_BLOCKED; none is silently skipped.
 * A failed call retains its executed prefix; stop or restore a checkpoint
 * before retrying. This object is not a dependency-suspension cursor.
 * $2052 audio, $2AEA palette allocation, $9A42 FG clear, $2208 movement.
 * No frame clock, cabinet input, actor rendering or VBlank is synthesized. */
typedef struct wf_r18_prematch wf_r18_prematch_t;
typedef wf_status_t (*wf_r18_prematch_effect_fn)(void *,uint32_t,
                                               wf_r18_prematch_t *);
struct wf_r18_prematch {
 uint8_t *ram;
 wf_r17_m68k_shadow_t *regs;
 uint32_t a[7];
 wf_r18_prematch_effect_fn effect;
 void *user;
 uint32_t dependency; /* first unavailable owner, zero after successful call */
};
/* One TAG presenter update: intact $A68A dispatch through the 24-entry $A7B4
 * table, returning at $A6A2 before rendering. Terminal state $18 returns OK
 * without another dispatch. Other invalid table indices return BLOCKED.
 * Caller owns $A654 join, actor programs, rendering, input/skip and teardown. */
wf_status_t wf_r18_prematch_tag_step_a68a(wf_r18_prematch_t *);
/* Saturn's sound output is currently silent. Shorten only speech waits in
 * that transport; divisor1 preserves the original arcade timing. */
void wf_r18_prematch_speech_divisor_set(unsigned divisor);
/* Original $ACB6 actor visual program; A0 must select one of nine actors.
 * Uses the twelve identity table windows, including shared-table aliases.
 * Retains the original mode-bit0 branch; supplies no Royal anchor setup. */
wf_status_t wf_r18_prematch_actor_step_acb6(wf_r18_prematch_t *);

enum { WF_R18_PREMATCH_MORE=0,WF_R18_PREMATCH_WAIT=1,WF_R18_PREMATCH_DONE=2,
       WF_R18_PREMATCH_BLOCKED=-1,WF_R18_PREMATCH_BAD_CONTEXT=-2 };
enum { WF_R18_PREMATCH_SYNC=0,WF_R18_PREMATCH_READ16=1 };
typedef wf_status_t (*wf_r18_prematch_io_fn)(void *,unsigned kind,
 uint32_t owner_pc,uint32_t address,uint16_t *value,wf_r18_prematch_t *);
typedef struct wf_r18_prematch_scene {
 uint32_t pc,blocked_dependency,saved_d0;
} wf_r18_prematch_scene_t;
/* Complete original $A654 join, visible loop and $A778 teardown, stopping at
 * its RTS $A7B2. The caller resumes at $C6C; this is not a direct $F8C handoff.
 * Additional synchronous effects: $98BA,$247C,$27B8,$2836,$2B58,$7506,
 * $2A06,$8EB6. TAG and Royal take their original distinct branches.
 * SYNC at $A728 requests an actual IRQ/ACK for RAM$1C006F bit1; unchanged
 * ACK returns WAIT. READ16 supplies the original four physical input words,
 * without remapping or inversion; the cursor applies NOT.W/AND.W $0030.
 * The IO callback may change live RAM/registers as the real IRQ would.
 * No synthetic frame counter, input, RNG seed, actor reset or anchor setup.
 * Failed effects/IO are fatal prefixes, as for the individual owner APIs. */
void wf_r18_prematch_scene_init(wf_r18_prematch_scene_t *);
int wf_r18_prematch_scene_step(wf_r18_prematch_scene_t *,wf_r18_prematch_t *,
                               wf_r18_prematch_io_fn,void *io_user);
#endif
