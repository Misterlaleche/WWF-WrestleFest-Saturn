#ifndef WF_NATIVE_HOST_H
#define WF_NATIVE_HOST_H
#include <stdbool.h>
#include "wf_native_driver.h"

typedef struct {
    uint32_t address;
    const uint8_t *data_be;
    uint32_t file_bytes, memory_bytes;
} wf_native_segment_t;
typedef struct {
    const wf_native_segment_t *segments;
    unsigned count;
} wf_native_image_t;
extern const wf_native_image_t wf_native_driver_image; /* generated from ELF */

/* Normal service context only. sound_enable must synchronously finish the SMPC
 * command. owner serializes with the game's SMPC/sound owner. Use uncached RAM.
 * Preload all segment data before SNDOFF; callbacks must not perform file I/O.
 * now_us is monotonic modulo 2^32. idle must allow it to advance, without
 * reentering this API. write16 accepts 68000 bus offsets, including SCSP regs.
 * barrier orders shared bus accesses. Default command deadline: 100 ms.
 * A timed-out command has unknown outcome; reboot before issuing another. */
typedef struct {
    bool (*sound_enable)(bool enable, void *user);
    void (*write16)(uint32_t address, uint16_t value, void *user);
    uint32_t (*now_us)(void *user);
    void (*wait_us)(uint32_t us, void *user);
    void (*idle)(void *user);
    bool (*enter_owner)(void *user);
    void (*leave_owner)(void *user);
    void (*barrier)(void *user);
    void *user;
} wf_native_platform_t;
typedef struct {
    volatile wf_native_mail_t *mail;
    wf_native_platform_t ops;
    uint32_t timeout_us;
    bool ready, poisoned;
} wf_native_host_t;

bool wf_native_image_valid(const wf_native_image_t *image);
bool wf_native_host_init(wf_native_host_t *host, volatile wf_native_mail_t *mail,
                         const wf_native_platform_t *ops);
bool wf_native_host_boot(wf_native_host_t *host, const wf_native_image_t *image);
bool wf_native_host_command(wf_native_host_t *host, uint16_t command,
                            uint16_t argument, uint16_t *value);
/* The vblank hook may only publish this tick. Firmware polls FM independently.
 * All setup, PCM table changes and copies run in normal service context. */
void wf_native_host_pcm_tick(wf_native_host_t *host);
#endif
