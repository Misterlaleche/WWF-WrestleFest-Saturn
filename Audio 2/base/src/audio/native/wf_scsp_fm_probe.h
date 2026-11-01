/* Experimental single-voice register emitter; not a complete sound driver. */
#ifndef WF_SCSP_FM_PROBE_H
#define WF_SCSP_FM_PROBE_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

enum {
    WF_FM_OPERATORS=4,
    WF_FM_WORDS=12,
    WF_FM_WAVE_BYTES=6144,
    WF_FM_WAVE_MIN=0x70000,
    WF_FM_WAVE_LIMIT=0x80000
};
typedef struct {
    uint32_t wave_start;
    uint16_t words[WF_FM_OPERATORS][WF_FM_WORDS];
} wf_scsp_fm_probe_patch_t;

/* 68000 bus offsets: RAM 0..0x7ffff, SCSP registers 0x100000 onward.
 * The caller owns slots 0..3 and grants this exclusive RAM span.
 * Callback runs in normal service context; never performs CD I/O. */
typedef bool (*wf_scsp_probe_write16_fn)(uint32_t address, uint16_t value, void *user);

bool wf_scsp_fm_probe_patch_valid(const wf_scsp_fm_probe_patch_t *patch);
bool wf_scsp_fm_probe_upload(const wf_scsp_fm_probe_patch_t *patch,
                            const uint8_t *wave_be, size_t bytes,
                            wf_scsp_probe_write16_fn write16, void *user);
bool wf_scsp_fm_probe_start(const wf_scsp_fm_probe_patch_t *patch,
                           wf_scsp_probe_write16_fn write16, void *user);
bool wf_scsp_fm_probe_stop(const wf_scsp_fm_probe_patch_t *patch,
                          wf_scsp_probe_write16_fn write16, void *user);
/* Direct MMIO callback. user points to the address of sound RAM in the
 * executing CPU's map: 0 for 68000 or a platform-selected SH-2 alias.
 * Platform initialization, bus serialization and ownership are external. */
bool wf_scsp_fm_probe_mmio_write16(uint32_t address, uint16_t value, void *user);
#endif
