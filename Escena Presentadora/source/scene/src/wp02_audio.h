#ifndef WP02_AUDIO_H
#define WP02_AUDIO_H
#include <stdbool.h>
#include <stdint.h>

typedef enum {
    WP02_AUDIO_DISABLED = 0,
    WP02_AUDIO_READY = 1,
    WP02_AUDIO_ERROR = 2
} wp02_audio_state_t;

/* Load before the scene. pcm_fad is the ISO FAD of cd/WP02PCM.PAK.
 * The caller supplies the unchanged SDRV.BIN image.
 * This module owns master SH-2 FRT; NTSC 320-pixel timing is used. */
bool wp02_audio_init(uint32_t pcm_fad, const uint8_t *driver, unsigned driver_bytes,
                     const uint8_t wrestler_ids[4]);

/* Load the original continuous0E background after audio_init, before timing.
 * Reserves all1MiB LWRAM plus704KiB HWRAM. Unchanged source is cached on replay.
 * Starts with first entry32. service runs once/display frame outside interrupts.
 * scene_done stops only music; SFX tails continue. No playback-time CD reads. */
bool wp02_audio_bgm_load(uint32_t fad, uint32_t bytes);
bool wp02_audio_service(void);
void wp02_audio_scene_done(void);
unsigned wp02_audio_bgm_refills(void);
unsigned wp02_audio_bgm_failures(void);
unsigned wp02_audio_bgm_max_service_us(void);

/* Accept raw sound-command low bytes from the ROM scene state machine.
 * Supported: 31,32,6D..75,7B..7D. Each call is one producer event.
 * No CD access occurs here or from the timer interrupt. */
bool wp02_audio_emit(uint8_t command);

/* Keep calling this after presenter exit so natural audio tails finish. */
void wp02_audio_vblank(void *work);

/* Immediate silence/reset for reload or teardown, not normal scene exit. */
void wp02_audio_stop(void);
wp02_audio_state_t wp02_audio_state(void);
unsigned wp02_audio_last_command(void);
unsigned wp02_audio_active_voices(void);
unsigned wp02_audio_sound_ram_end(void);
/* Monotonic timer since first initialization, independent of rendering.
 * No getter resets the FRT. Raw ticks are suitable for an external exact
 * Saturn oscillator ratio. time_us uses the official Mednafen NTSC320
 * rational 1746818182/(65*8) Hz; physical oscillator tolerance is separate.
 * This API assumes the display remains in NTSC 320-pixel clock mode. */
uint64_t wp02_audio_time_frt_ticks(void);
uint64_t wp02_audio_time_us(void);
uint32_t wp02_audio_frt_compare_ticks(void);
#endif
