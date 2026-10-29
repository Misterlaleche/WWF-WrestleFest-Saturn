#ifndef WP01_SCENE_H
#define WP01_SCENE_H
#include <stdint.h>

/* ROM $7B70/$7F0E aisle scene. No ring-announcement/gameplay logic.
 * init(id0,id1,seed): inputs are SELECTED internal ROM IDs, not Saturn's
 * legacy logical order. init applies ROM pair ordering (Hogan right,
 * DiBiase left), so actor[0] is physical left and actor[1] physical right.
 * 0 Hogan,1 Warrior,2 Jake,3 Bossman,4 Hawk,5 Animal,6 Slaughter,7 Perfect,
 * 8 Earthquake,9 DiBiase,10 Smash,11 Crush. Selectable IDs exclude4/5.
 *
 * init produces frame0. Render state, then step once per arcade update.
 * Frames0..23 reveal;24..343 walk;344..367 exit wipe;368..400 black hold;
 * frame401 terminal cleanup. A step AFTER rendering401 sets done, leaving
 * frame at401. The complete sequence is402 output updates.
 * MAME frame1277 is coreframe0; first computed actor state1301 isframe24.
 * Original buffered sprites display one frame after computed actor state;
 * renderer owns that latency. Do not also delay wp01_scene_step.
 * Six blank setup frames preceding1277 are outside this core.
 * world_y is signed16.16. sx/sy are actor fields+14/+16; anchor_y=232-sy.
 * Use anchor_x/y with wp25 expanded-piece offsets; no extra16px bias.
 */
enum { WP01_SCENE_FRAMES=402, WP01_WIPE_NONE=-1 };

typedef struct {
    uint16_t internal_id, type, selector;
    int16_t world_x, base_z;
    int32_t world_y;
    int16_t sx, sy, anchor_x, anchor_y, correction_y;
    uint16_t counter, phase;
    uint8_t visible;
} wp01_scene_actor_t;

typedef struct {
    int16_t x, y;
    uint16_t selector, counter, phase;
    uint8_t visible;
} wp01_scene_light_t;

/* Optional entropy callback. It is called exactly when ROM$8122 would
 * call$21B4 (after a four-phase cycle for types40/41/47/49). Return a word
 * whose bit0 chooses next cycle:0=>selectors4..7,1=>selectors0..3.
 * It permits replay of a traced arcade entropy stream without altering
 * movement or phase timing. The default uses the$21B4 rotate/add routine
 * with native scene entropy; a fresh native seed does not reproduce the
 * original CPU's unrelated live D1..D7 registers.
 */
typedef uint32_t (*wp01_scene_rng_hook_t)(void *context, unsigned actor,
                                        unsigned movement_update);
typedef struct {
    uint16_t frame, movement_updates;
    uint8_t done;
    int16_t wipe_tile;
    uint32_t rng_state;
    wp01_scene_actor_t actor[2];
    wp01_scene_light_t light;
    wp01_scene_rng_hook_t rng_hook;
    void *rng_context;
} wp01_scene_t;

void wp01_scene_init(wp01_scene_t *s, unsigned id0, unsigned id1, uint32_t rngseed);
void wp01_scene_step(wp01_scene_t *s);
void wp01_scene_set_rng_hook(wp01_scene_t *s, wp01_scene_rng_hook_t hook, void *context);
/* Exact$21B4 arithmetic when supplied the actual live register sum and
 * global longword$1C0080. Arithmetic wraps modulo2^32. */
uint32_t wp01_rng_21b4(uint32_t *state, uint32_t d1_to_d7_sum, uint32_t global_timer);

#endif
