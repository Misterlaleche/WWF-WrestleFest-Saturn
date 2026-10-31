#ifndef WF_VIDEO_PLAN_H
#define WF_VIDEO_PLAN_H
#include <stdint.h>
#include <stdbool.h>
typedef struct {
 uint32_t maps[2][1024],text[4096],palette[1024];
 uint16_t code_slot[2][4096],slot_code[2][1024];
 uint8_t needed[4096];
} wf_video_plan_t;
typedef struct {uint16_t code;int16_t x,y;uint8_t bank,flip;} wf_video_piece_t;
void wf_video_plan_init(wf_video_plan_t *);
bool wf_video_plan_map(wf_video_plan_t *,unsigned layer,const uint8_t *raw,
 bool (*upload)(void *,unsigned,unsigned,unsigned),void *);
void wf_video_plan_text(wf_video_plan_t *,const uint8_t *raw);
void wf_video_plan_palette(uint32_t *,const uint8_t *raw);
bool wf_video_plan_sprites(const uint8_t *raw,unsigned records,wf_video_piece_t *,unsigned capacity,unsigned *count);
#endif
