#ifndef WWF_RING_ROPE_ROM_EXACT_H
#define WWF_RING_ROPE_ROM_EXACT_H
typedef unsigned char wwf_u8;
typedef unsigned short wwf_u16;

enum { WWF_ROPE_LEFT=0, WWF_ROPE_RIGHT=1 };
enum { WWF_ROPE_STATE_STATIC=0, WWF_ROPE_STATE_1=1, WWF_ROPE_STATE_2=2, WWF_ROPE_STATE_3=3, WWF_ROPE_STATE_4=4 };
enum { WWF_ROPE_RECORDS=4 };

typedef struct {
    wwf_u16 state_word; /* ROM +0x1C: low nibble dispatch, bit7 init latch */
    wwf_u16 timer;      /* ROM +0x22 */
    wwf_u16 phase;      /* ROM +0x24 */
    wwf_u16 gfx_set;    /* ROM +0x02; always 0x000E here */
    wwf_u16 world_x;    /* ROM +0x06 */
    wwf_u16 world_y;    /* ROM +0x0A */
    wwf_u16 world_z;    /* ROM +0x0E */
    wwf_u16 class_word; /* ROM +0x12; low byte bit1 selects paired phase table */
    wwf_u16 orient;     /* ROM +0x2E */
    wwf_u8 selector;    /* ROM +0x05 */
} wwf_rope_record_t;

typedef struct { wwf_rope_record_t r[WWF_ROPE_RECORDS]; } wwf_rope_quartet_t;

void wwf_rope_quartet_reset(wwf_rope_quartet_t *q);
void wwf_rope_write_state(wwf_rope_quartet_t *q, unsigned record, wwf_u16 state);
void wwf_rope_write_pair_state(wwf_rope_quartet_t *q, unsigned side, wwf_u16 state);
void wwf_rope_step_record(wwf_rope_record_t *r);
void wwf_rope_step(wwf_rope_quartet_t *q);
unsigned wwf_rope_record_side(unsigned record);

#endif
