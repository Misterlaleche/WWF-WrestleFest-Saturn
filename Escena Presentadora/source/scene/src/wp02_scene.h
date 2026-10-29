#ifndef WP02_SCENE_H
#define WP02_SCENE_H
#include <stdint.h>
#define WP02_WRESTLER_SLOTS 9u
#define WP02_SOUND_CAPACITY 8u
/* RAW ROM internal IDs / graphics sets: Hogan, Warrior, Jake, Boss Man,
   Hawk, Animal, Slaughter, Perfect, Earthquake, DiBiase, Smash, Crush.
   These are already raw actor +0x57 / +0x02 IDs. Do NOT apply the old
   Saturn UI permutation a second time. Arcade selectable IDs exclude4/5;
   Demolition uses the canonical ordered pair10/11 on the same team. */
typedef struct {
    uint32_t active, type, selector;
    uint32_t x, y, z; /* Native machine coordinates, unsigned 16.16. */
    uint32_t state, counter, phase, counter1e, flags33;
} wp02_actor;
typedef struct {
    uint32_t ticks, done, terminal_pending, error;
    uint32_t rng, rng_global80, rng_mix[7], group_flag;
    uint32_t sound_count;
    uint32_t raw_commands[WP02_SOUND_CAPACITY];
    uint32_t effective_commands[WP02_SOUND_CAPACITY];
    wp02_actor presenter;
    wp02_actor wrestlers[WP02_WRESTLER_SLOTS];
    wp02_actor referee;
    uint32_t names_visible;
} wp02_scene;
/* Returns 1 for four distinct valid IDs. The supplied seed is explicit
   scene/test entropy, never advertised as a ROM-defined cold-boot seed. */
int wp02_init(wp02_scene *scene, const uint8_t identities[4], uint32_t seed);
/* 1 = a visible prematch display update emitted; 0 = terminal clear/done
   (or invalid input). After state18's final display, call once more to clear
   the presenter without consuming an extra original display-loop update. */
int wp02_tick(wp02_scene *scene);
uint32_t wp02_rng_step(uint32_t state, const uint32_t mix[7], uint32_t global80);
#endif
