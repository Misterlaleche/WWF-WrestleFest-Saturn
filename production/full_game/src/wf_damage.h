#ifndef WF_DAMAGE_H
#define WF_DAMAGE_H
#include <stdint.h>
enum { WF_DAMAGE_BANDS=15,WF_DAMAGE_WIDTH=320,WF_DAMAGE_HEIGHT=240,WF_DAMAGE_RECTS_MAX=150 };
typedef struct {uint16_t x,y,width,height;} wf_damage_rect_t;
typedef struct {
 uint32_t history[2][WF_DAMAGE_BANDS],current[WF_DAMAGE_BANDS];
 unsigned full_clears;
} wf_damage_t;
/* Manual double-buffer transport only. Each successful submission completes
 * its swap before the next begin. Union of both prior footprints is safe for
 * either buffer. Reset forces two full clears of any inherited picture.
 * begin without commit is an aborted, unsubmitted command list. */
void wf_damage_init(wf_damage_t *);
unsigned wf_damage_begin(wf_damage_t *,wf_damage_rect_t [WF_DAMAGE_RECTS_MAX]);
void wf_damage_add(wf_damage_t *,int16_t,int16_t,uint16_t,uint16_t);
void wf_damage_commit(wf_damage_t *);
#endif
