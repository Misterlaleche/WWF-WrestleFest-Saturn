#ifndef WF_AUDIO_LIVE_VM_H
#define WF_AUDIO_LIVE_VM_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "z80.h"
#include "opm.h"

enum { WF_LIVEVM_MASTER_HZ=3579545u, WF_LIVEVM_OUT_HZ=44100u, WF_LIVEVM_OKI_HZ=8000u };

typedef struct { uint32_t offset, bytes; uint8_t valid; } wf_livevm_asset_t;
typedef struct {
    uint8_t playing, sample_id, volume;
    uint32_t pos, bytes, offset;
    int8_t current;
} wf_livevm_voice_t;

typedef struct {
    z80 cpu;
    opm_t opm;
    const uint8_t *sound_rom; size_t sound_rom_bytes;
    const uint8_t *audpcm; size_t audpcm_bytes;
    wf_livevm_asset_t bank0[128];
    uint8_t ram[0x800];
    uint8_t latch, ym_addr, ym_status, ym_tb, ym_ctl, bank;
    uint64_t next_tb;
    uint64_t rendered_cycle;
    uint64_t origin_cycle;
    uint64_t out_phase, oki_phase;
    int32_t y[2];
    wf_livevm_voice_t voices[4];
    int pending;
    uint64_t samples_total;
    uint8_t started;
    uint8_t error;
    uint8_t *render_dst;
    uint32_t render_want;
    uint32_t render_made;
} wf_audio_live_vm_t;

bool wf_audio_live_vm_init(wf_audio_live_vm_t *v,
                           const uint8_t *sound_rom, size_t sound_rom_bytes,
                           const uint8_t *audpcm, size_t audpcm_bytes,
                           const wf_livevm_asset_t bank0[128]);
bool wf_audio_live_vm_start(wf_audio_live_vm_t *v, uint8_t command);
bool wf_audio_live_vm_render_be(wf_audio_live_vm_t *v, uint8_t *dst, uint32_t samples);
#endif
