#ifndef WWF_ARCADE_ATTRACT_BIO_H
#define WWF_ARCADE_ATTRACT_BIO_H
#include <stdbool.h>
#include <stdint.h>

typedef struct {
 uint8_t character;          /* ROM $8E9A, states 2-13 */
 uint8_t composition_set;    /* $30 + character */
 uint8_t character_palette;  /* $10 + character, loaded after common $1E */
 uint8_t common_palette;     /* $1E */
 uint16_t text_ids[3];       /* 6, 7+(state-2), $40+(state-2) */
} arcade_attract_bio_desc_t;

typedef struct { uint8_t pose; uint8_t toggle; uint8_t countdown; bool visible; } arcade_attract_bio_anim_t;

bool arcade_attract_bio_desc(unsigned state,arcade_attract_bio_desc_t *out);
void arcade_attract_bio_anim_begin(arcade_attract_bio_anim_t *a);
/* Exact $8DFC-$8E46 policy. rng_low_nibble is D0&$0f returned at the reload point. */
void arcade_attract_bio_anim_tick(arcade_attract_bio_anim_t *a,uint16_t phase_counter,uint8_t rng_low_nibble);
#endif
