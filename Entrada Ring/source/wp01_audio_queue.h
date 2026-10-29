#ifndef WP01_AUDIO_QUEUE_H
#define WP01_AUDIO_QUEUE_H
#include <stdint.h>
#include <stdbool.h>
typedef void (*wp01_audio_write_fn)(void *,uint16_t,uint16_t);
typedef struct {
    const uint8_t *initial,*events;
    uint32_t initial_count,event_count,duration,next,elapsed,max_lateness,max_burst;
    wp01_audio_write_fn write;
    void *context;
} wp01_audio_queue_t;
bool wp01_audio_queue_bind(wp01_audio_queue_t *,const void *,uint32_t,wp01_audio_write_fn,void *);
void wp01_audio_queue_restart(wp01_audio_queue_t *);
uint32_t wp01_audio_queue_advance(wp01_audio_queue_t *,uint32_t);
#endif
