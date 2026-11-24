#ifndef WF_JO_AUDIO_H
#define WF_JO_AUDIO_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "wf_runtime.h"

/* All cartridge pointers are borrowed for the lifetime of this backend. Jo is
 * the only SCSP/68000/SMPC owner. No native Audio 2 firmware is linked. */
#define WF_JO_AUDIO_DECODE_BUDGET 2048u
#define WF_JO_AUDIO_PREFIX_WORKSPACE_BYTES 32768u
#define WF_JO_AUDIO_CART_PREFIX_BYTES 114688u
typedef struct {
    uint32_t magic, initialized, fault, pack_bytes;
    uint32_t commands_received, commands_applied, last_raw, last_command;
    uint32_t queue_depth, queue_peak, queue_overflows, priority_rejects;
    uint32_t physical_starts, f1_skips, cleanup_stops, selected_voice;
    uint32_t resident_hits, cache_hits, cache_misses, pcm_underruns;
    uint32_t decode_samples, max_decode_samples_service, decode_total_ms, decode_max_ms;
    uint32_t resident_pcm_bytes, dynamic_pcm_bytes, service_max_gap_ms, late_timer_ticks;
    uint32_t bank_changes, active_bank_changes, launch_max_delay_ms;
    uint32_t music_prepared_mask, music_missing_mask, music_errors, music_command;
    uint32_t music_exhaustions, music_natural_ends, music_underruns, unsupported_fades;
    uint32_t music_prepare_max_ms, music_start_max_delay_ms, music_pending_ms;
    uint32_t music_prefetch_mask, music_prefetch_bytes, invalid_bank_starts;
    uint32_t music_command_max_ms, music_service_max_ms;
    uint32_t pcm_underrun_sample, pcm_underrun_decoded, pcm_underrun_elapsed_frames;
    uint32_t music_io_errors;
    uint32_t preserved_primes;
    uint32_t cooperative_services, cooperative_max_ms, command_queue_max_ms;
    uint32_t music_fades, music_fade_completions, music_fade_volume;
} wf_jo_audio_diagnostic_t;
extern volatile wf_jo_audio_diagnostic_t wf_jo_audio_diagnostic;
bool wf_jo_audio_pack_valid(const void *pack, size_t bytes);
bool wf_jo_audio_init(const void *sfx_pack, size_t bytes);
/* Terminal fault only: immediate SDK mute, abandon pending commands/voices,
 * preserve borrowed memory and reject further init/service until a reboot.
 * Must precede any video drain, filesystem operation or diagnostic output. */
void wf_jo_audio_emergency_stop(void);
/* Call while loading, after init and before staging/priming any music. With
 * >=112 KiB of 4-byte-aligned borrowed storage, cache 64 KiB of 0B then 48 KiB
 * of 08, ignoring the optional workspace and allocating no heap. The existing
 * 96 KiB tier keeps 64 KiB of 0B and 32 KiB of 08. Smaller leases keep 32 KiB
 * of each cue: 08 prefers the workspace, then remaining cart, then Jo.
 * Each successful prefix is immutable for backend life. Retry uses fixed
 * offsets, never overwrites an existing prefix, and returns the cached ID mask.
 * Invalid/overlapping leases and calls during active/pending music do no work. */
uint32_t wf_jo_audio_prefetch_music(void *storage, size_t bytes);
/* Optional caller-owned 32 KiB cue08 prefix. Call after init and before
 * prefetch; storage must remain valid and 4-byte aligned for backend life.
 * Not needed when prefetch is given WF_JO_AUDIO_CART_PREFIX_BYTES or more. */
bool wf_jo_audio_set_prefix_workspace(void *storage, size_t bytes);
/* Last loading operation, after all ordinary CD reads and prefix caching. */
bool wf_jo_audio_prime_music(uint8_t command);
/* Main-thread work boundary, before presentation or after a complete asset
 * decode. Services queued commands immediately, otherwise at most once per
 * four milliseconds. Uses the normal bounded PCM budget, reads no assets,
 * never synchronizes fields and safely ignores nested service requests. */
bool wf_jo_audio_service_cooperate(void);
bool wf_r18_scsp_boot(volatile uint32_t *video_clock, const void *elf, size_t bytes);
bool wf_r18_scsp_smpc_owned(void);
void wf_r18_scsp_vblank(void);
bool wf_r18_scsp_service(void);
wf_status_t wf_r18_scsp_audio(wf_runtime_t *runtime, uint16_t raw, void *user);
#ifdef WF_JO_AUDIO_HOST
bool wf_jo_audio_test_decode_catalog(const void *, size_t, const void *, size_t);
#endif
#endif
