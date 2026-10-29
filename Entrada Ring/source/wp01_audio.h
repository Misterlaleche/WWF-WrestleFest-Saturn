#ifndef WP01_AUDIO_H
#define WP01_AUDIO_H
#include <stdint.h>
#include <stdbool.h>
/* Native SCSP operators and individual original OKI samples. No mixed music PCM. */
typedef struct {
    uint32_t magic,interrupts,elapsed_ticks,event_index,max_lateness_ticks;
    uint32_t max_handler_ticks,max_burst,key_guards,timer_rearms,playing;
} wp01_audio_debug_t;
extern volatile wp01_audio_debug_t wp01_audio_debug;
void wp01_audio_init(void);
bool wp01_audio_bank_load(const void *,uint32_t);
bool wp01_audio_program_load(const void *,uint32_t);
void wp01_audio_prepare(void);
void wp01_audio_start(void);
void wp01_audio_stop(void);
void wp01_audio_service(void);
uint32_t wp01_audio_written(void);
#endif
