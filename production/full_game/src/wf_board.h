#ifndef WF_BOARD_H
#define WF_BOARD_H
#include <stdint.h>
#include <stdbool.h>
typedef struct {
 uint16_t ports[4],line,backdrop,last_sound;
 uint8_t scroll[0x20],control[0x20];
 uint32_t frames,sound_count;
 bool irq2,irq3;
} wf_board_t;
void wf_board_init(wf_board_t *,uint8_t dip1,uint8_t dip2);
void wf_board_input(wf_board_t *,unsigned port,uint8_t pressed,bool coin);
bool wf_board_read(const wf_board_t *,uint32_t address,uint32_t *,unsigned bytes);
bool wf_board_write(wf_board_t *,uint32_t address,uint32_t value,unsigned bytes);
bool wf_board_high(const wf_board_t *);
/* Ordered board transitions:0,8,16,...240,248,256. Physical pacing belongs
 * to the platform at frame0; this emits no guessed CPU cycles or gameplay. */
unsigned wf_board_advance(wf_board_t *);
/* Admitted highest level first; ACK comes from original MMIO writes. The
 * caller rechecks SR/generation before another dispatch. */
bool wf_board_dispatch(wf_board_t *,uint16_t sr,bool (*irq)(void *,unsigned),void *);
#endif
