#ifndef WF_MUSIC_TIME_H
#define WF_MUSIC_TIME_H
#include <stdint.h>
typedef struct {uint64_t cycle;uint32_t fraction;uint16_t frame;} wf_music_time_t;
static inline uint16_t wf_music_time_advance(wf_music_time_t *t,uint64_t cycle)
{
 uint64_t delta=cycle-t->cycle;t->cycle=cycle;
 if(delta<=90000u){
  uint32_t numerator=(uint32_t)delta*44100u+t->fraction;
  t->frame=(uint16_t)(t->frame+numerator/3579545u);t->fraction=numerator%3579545u;
 }else{
  uint64_t numerator=delta*44100u+t->fraction;
  t->frame=(uint16_t)(t->frame+numerator/3579545u);t->fraction=(uint32_t)(numerator%3579545u);
 }
 return t->frame;
}
#endif
