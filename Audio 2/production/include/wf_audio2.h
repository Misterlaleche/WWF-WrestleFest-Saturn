#ifndef WF_AUDIO2_H
#define WF_AUDIO2_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "wf_native_host.h"
#include "wf_music_module.h"

/* Public API version; this does not change either historical wire ABI.
 * All calls except vblank belong to one serialized normal-service owner.
 * command/service never wait for a mailbox acknowledgement. BUSY means that
 * nothing was published: the original producer must retain and retry it.
 * Never create a second gameplay producer to compensate for BUSY.
 */
#define WF_AUDIO2_API_VERSION 1u
typedef enum {
    WF_AUDIO2_CLOSED=0, WF_AUDIO2_STARTING, WF_AUDIO2_RUNNING,
    WF_AUDIO2_STOPPING, WF_AUDIO2_STOPPED, WF_AUDIO2_FAULT
} wf_audio2_state_t;
typedef enum {
    WF_AUDIO2_ACCEPTED=0, WF_AUDIO2_BUSY=1, WF_AUDIO2_NOT_READY=2,
    WF_AUDIO2_UNSUPPORTED=3, WF_AUDIO2_FAILED=4
} wf_audio2_result_t;
typedef enum {
    WF_AUDIO2_ERROR_NONE=0, WF_AUDIO2_ERROR_ARGUMENT=1,
    WF_AUDIO2_ERROR_RESOURCE=2, WF_AUDIO2_ERROR_BOOT=3,
    WF_AUDIO2_ERROR_WORKER_START=4, WF_AUDIO2_ERROR_WORKER=5,
    WF_AUDIO2_ERROR_NATIVE=6, WF_AUDIO2_ERROR_MAIL_TIMEOUT=7,
    WF_AUDIO2_ERROR_START_TIMEOUT=8, WF_AUDIO2_ERROR_STOP_TIMEOUT=9,
    WF_AUDIO2_ERROR_PROTOCOL=10, WF_AUDIO2_ERROR_WORKER_STALLED=11,
    WF_AUDIO2_ERROR_DSP_STALLED=12
} wf_audio2_error_t;
typedef struct {
    const void *firmware_elf;
    size_t firmware_bytes;
    const uint8_t *sfx_pack;
    size_t sfx_bytes;
    uint32_t workspace;
    volatile wf_native_mail_t *mail;
    volatile wf_music_module_shared_t *music;
    wf_native_platform_t platform;
    /* start must prepare the linked module, purge the master cache over the
     * owned hot allocation and launch the uncached slave entry trampoline.
     * quiescent may return true only after that entry actually returned.
     * It must never merely clear Yaul's entry pointer and claim a stopped CPU.
     */
    bool (*worker_start)(uint32_t workspace, void *user);
    bool (*worker_quiescent)(void *user);
    void *worker_user;
} wf_audio2_config_t;
typedef struct {
    uint32_t api_version;
    wf_audio2_state_t state;
    wf_audio2_error_t error;
    uint32_t worker_error, accepted_commands, priority_rejections;
    uint32_t max_mail_latency_us;
    uint16_t last_word, last_native_result, worker_state, native_state;
    uint16_t command_queue_used, mail_pending;
} wf_audio2_status_t;
typedef struct {
    wf_audio2_config_t config;
    wf_native_host_t host;
    wf_audio2_status_t status;
    uint32_t state_started_us, mail_started_us;
    uint16_t pending_sequence;
    bool pending, stop_sent;
} wf_audio2_t;

/* Zero-initialize context before its first use. Cold init can wait for SMPC
 * and the native boot handshake; call it before enabling scene producers.
 * It verifies the complete pack CRC and ELF windows before any hardware write.
 */
bool wf_audio2_init(wf_audio2_t *, const wf_audio2_config_t *);
wf_audio2_result_t wf_audio2_command(wf_audio2_t *, uint16_t original_word);
void wf_audio2_service(wf_audio2_t *);
void wf_audio2_vblank(wf_audio2_t *, uint16_t video_frame);
void wf_audio2_status(const wf_audio2_t *, wf_audio2_status_t *);
/* System shutdown, not an invented implementation of ROM fade 0x1F.
 * Continue service until STOPPED. Recovery reboots only after the worker
 * really returned; a wedged slave requires a platform cold restart.
 */
void wf_audio2_stop(wf_audio2_t *);
bool wf_audio2_recover(wf_audio2_t *);
bool wf_audio2_resource_valid(const uint8_t *, size_t);
#endif
