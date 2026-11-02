/* Host experiment only. This does not install a native resource ABI. */
#ifndef WF_OKI_ADDRESS_PROTOTYPE_H
#define WF_OKI_ADDRESS_PROTOTYPE_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

enum { WF_OKI_WINDOW = 0x40000, WF_OKI_RING = 4096,
       WF_OKI_CHECKPOINT_STRIDE = 128, WF_OKI_CHECKPOINTS = 33 };

typedef struct {
    const uint8_t *pack, *extension;
    uint32_t pack_bytes, extension_bytes;
} wf_oki_map_t;

/* Clear this cache when binding a different map. It owns no source bytes. */
typedef struct {
    uint32_t start, end, offset;
    uint8_t bank, valid;
} wf_oki_span_t;

bool wf_oki_map_init(wf_oki_map_t *, const uint8_t *, size_t,
                     const uint8_t *, size_t);
bool wf_oki_map_sample(const wf_oki_map_t *, unsigned bank, unsigned sample,
                       uint32_t *start, uint32_t *nibbles);
bool wf_oki_map_read(const wf_oki_map_t *, wf_oki_span_t *, unsigned bank,
                     uint32_t address, uint8_t *value);

/* Signal/index are the complete irreversible ADPCM history. Position and bank
 * are supplied explicitly; tag belongs only to the checkpoint store. */
typedef struct {
    int16_t signal;
    uint8_t index, tag;
} wf_oki_checkpoint_t;

bool wf_oki_decode_at(const wf_oki_map_t *, wf_oki_span_t *, unsigned bank,
                      uint32_t start, uint32_t nibbles, uint32_t position,
                      wf_oki_checkpoint_t *, int16_t *pcm);

typedef struct {
    wf_oki_span_t cache;
    uint32_t start, nibbles, played, prepared, anchor_position;
    wf_oki_checkpoint_t state, anchor, checkpoints[WF_OKI_CHECKPOINTS];
    uint8_t bank, ready;
} wf_oki_ring_t;

typedef struct { uint32_t restore_samples, repair_samples; } wf_oki_work_t;

/* Caller supplies 4097 int16 slots: 4096 circular samples plus the existing
 * interpolation guard, which mirrors slot zero. All positions count 8 kHz
 * PCM samples from voice start, never 44.1 kHz SCSP frames.
 * A bank write applies before the next sample at played. Calls must be
 * monotonic and serialized; this is not a hardware handoff implementation. */
bool wf_oki_ring_start(wf_oki_ring_t *, const wf_oki_map_t *, unsigned bank,
                       unsigned sample);
bool wf_oki_ring_fill(wf_oki_ring_t *, const wf_oki_map_t *, int16_t *pcm,
                      uint32_t target);
bool wf_oki_ring_commit(wf_oki_ring_t *, uint32_t played);
bool wf_oki_ring_bank(wf_oki_ring_t *, const wf_oki_map_t *, int16_t *pcm,
                      unsigned bank, wf_oki_work_t *);
#endif
