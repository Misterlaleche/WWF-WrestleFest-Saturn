#include "wf_oki_address.h"

static uint16_t be16(const uint8_t *p)
{ return (uint16_t)((unsigned)p[0] * 256u + p[1]); }
static uint32_t be24(const uint8_t *p)
{ return (uint32_t)p[0] * 65536u + (uint32_t)p[1] * 256u + p[2]; }
static uint32_t be32(const uint8_t *p)
{ return (uint32_t)be16(p) * 65536u + be16(p + 2); }

static const uint8_t *sample_entry(const wf_oki_map_t *map, unsigned index)
{
    unsigned slot = index < 73u ? index + 1u : index + 2u;
    return map->pack + 32u + slot * 8u;
}

bool wf_oki_map_init(wf_oki_map_t *map, const uint8_t *pack, size_t pack_bytes,
                     const uint8_t *extension, size_t extension_bytes)
{
    if (!map) return false;
    *map = (wf_oki_map_t){0};
    if (!pack || !extension || pack_bytes < 960u || pack_bytes > 0x6c000u ||
        extension_bytes < 16u || extension_bytes > 0x6c000u - pack_bytes ||
        be32(pack) != 0x57465358u || be16(pack + 4) != 1u ||
        be16(pack + 6) != 116u || be32(pack + 8) != pack_bytes ||
        be32(pack + 12) != 32u || be32(pack + 24) > pack_bytes ||
        be32(extension) != 0x57464241u || be16(extension + 4) != 1u ||
        be16(extension + 6) != 114u || be16(extension + 8) != 1u ||
        be16(extension + 10) != 342u) return false;
    /* This experiment consumes the audit's one-extra-range construction.
     * Its offsets are relative to the unchanged pack followed by this tail.
     * Pack CRC/ROM identity remains the private runner's responsibility. */
    uint32_t metadata_bytes = 16u + 342u + 8u;
    uint32_t payload_bytes = be32(extension + 12);
    if (payload_bytes > extension_bytes ||
        ((metadata_bytes + payload_bytes + 1u) & ~1u) != extension_bytes)
        return false;
    wf_oki_map_t candidate = {pack, extension, (uint32_t)pack_bytes,
                             (uint32_t)extension_bytes};
    for (unsigned i = 0; i < 114u; i++) {
        const uint8_t *entry = sample_entry(&candidate, i);
        uint32_t start = be24(extension + 16u + i * 3u);
        uint32_t bytes = be16(entry + 4), offset = be32(entry);
        if (!bytes || start >= WF_OKI_WINDOW || bytes > WF_OKI_WINDOW - start ||
            offset < be32(pack + 24) || offset > pack_bytes ||
            bytes > pack_bytes - offset) return false;
    }
    const uint8_t *extra = extension + 358u;
    uint32_t physical = be24(extra), bytes = be16(extra + 3), offset = be24(extra + 5);
    uint32_t start = physical & (WF_OKI_WINDOW - 1u);
    if (physical >= 2u * WF_OKI_WINDOW || !bytes ||
        bytes > WF_OKI_WINDOW - start || bytes != payload_bytes ||
        offset != pack_bytes + metadata_bytes) return false;
    *map = candidate;
    return true;
}
bool wf_oki_map_sample(const wf_oki_map_t *map, unsigned bank, unsigned sample,
                       uint32_t *start, uint32_t *nibbles)
{
    if (!map || !map->pack || !start || !nibbles || bank > 1u || !sample ||
        sample > (bank ? 41u : 73u)) return false;
    unsigned index = (bank ? 73u : 0u) + sample - 1u;
    *start = be24(map->extension + 16u + index * 3u);
    *nibbles = 2u * be16(sample_entry(map, index) + 4);
    return true;
}
bool wf_oki_map_read(const wf_oki_map_t *map, wf_oki_span_t *cache, unsigned bank,
                     uint32_t address, uint8_t *value)
{
    if (!map || !map->pack || !cache || !value || bank > 1u) return false;
    address &= WF_OKI_WINDOW - 1u;
    if (!cache->valid || cache->bank != bank ||
        address < cache->start || address >= cache->end) {
        cache->valid = 0;
        unsigned first = bank ? 73u : 0u, count = bank ? 41u : 73u;
        for (unsigned i = first; i < first + count; i++) {
            const uint8_t *entry = sample_entry(map, i);
            uint32_t start = be24(map->extension + 16u + i * 3u);
            uint32_t bytes = be16(entry + 4);
            if (address >= start && address - start < bytes) {
                *cache = (wf_oki_span_t){start, start + bytes, be32(entry),
                                        (uint8_t)bank, 1};
                break;
            }
        }
        if (!cache->valid) {
            const uint8_t *extra = map->extension + 358u;
            uint32_t physical = be24(extra), start = physical & (WF_OKI_WINDOW - 1u);
            uint32_t bytes = be16(extra + 3);
            if ((physical >> 18) != bank || address < start || address - start >= bytes)
                return false;
            *cache = (wf_oki_span_t){start, start + bytes, be24(extra + 5),
                                    (uint8_t)bank, 1};
        }
    }
    uint32_t offset = cache->offset + address - cache->start;
    *value = offset < map->pack_bytes ? map->pack[offset]
                                      : map->extension[offset - map->pack_bytes];
    return true;
}

bool wf_oki_decode_at(const wf_oki_map_t *map, wf_oki_span_t *cache, unsigned bank,
                      uint32_t start, uint32_t nibbles, uint32_t position,
                      wf_oki_checkpoint_t *state, int16_t *pcm)
{
    static const uint16_t steps[49] = {
        16,17,19,21,23,25,28,31,34,37,41,45,50,55,60,66,73,80,88,97,
        107,118,130,143,157,173,190,209,230,253,279,307,337,371,408,
        449,494,544,598,658,724,796,876,963,1060,1166,1282,1411,1552
    };
    static const int8_t shifts[8] = {-1,-1,-1,-1,2,4,6,8};
    if (!state || !pcm || state->index > 48u ||
        state->signal < -2048 || state->signal > 2047 || bank > 1u)
        return false;
    if (position >= nibbles) { *pcm = 0; return true; }
    uint8_t byte;
    if (!wf_oki_map_read(map, cache, bank, start + position / 2u, &byte)) return false;
    unsigned nibble = position & 1u ? byte & 15u : byte >> 4;
    int step = steps[state->index], delta = step >> 3;
    if (nibble & 1u) delta += step >> 2;
    if (nibble & 2u) delta += step >> 1;
    if (nibble & 4u) delta += step;
    int signal = state->signal + (nibble & 8u ? -delta : delta);
    if (signal > 2047) signal = 2047;
    if (signal < -2048) signal = -2048;
    int index = state->index + shifts[nibble & 7u];
    if (index < 0) index = 0;
    if (index > 48) index = 48;
    state->signal = (int16_t)signal;
    state->index = (uint8_t)index;
    *pcm = (int16_t)(signal * 16);
    return true;
}

bool wf_oki_ring_start(wf_oki_ring_t *ring, const wf_oki_map_t *map, unsigned bank,
                       unsigned sample)
{
    uint32_t start, nibbles;
    if (!ring || !wf_oki_map_sample(map, bank, sample, &start, &nibbles)) return false;
    *ring = (wf_oki_ring_t){.start = start, .nibbles = nibbles,
                            .bank = (uint8_t)bank, .ready = 1};
    return true;
}

static void checkpoint_save(wf_oki_ring_t *ring)
{
    if (ring->prepared % WF_OKI_CHECKPOINT_STRIDE) return;
    uint32_t number = ring->prepared / WF_OKI_CHECKPOINT_STRIDE;
    wf_oki_checkpoint_t *slot = &ring->checkpoints[number % WF_OKI_CHECKPOINTS];
    *slot = ring->state;
    slot->tag = (uint8_t)number;
}

bool wf_oki_ring_fill(wf_oki_ring_t *ring, const wf_oki_map_t *map, int16_t *pcm,
                      uint32_t target)
{
    if (!ring || !ring->ready || !pcm || target < ring->prepared ||
        target < ring->played || target - ring->played > WF_OKI_RING) return false;
    while (ring->prepared < target) {
        checkpoint_save(ring);
        int16_t sample;
        if (!wf_oki_decode_at(map, &ring->cache, ring->bank, ring->start,
                             ring->nibbles, ring->prepared, &ring->state, &sample))
            return false;
        uint32_t slot = ring->prepared % WF_OKI_RING;
        pcm[slot] = sample;
        if (!slot) pcm[WF_OKI_RING] = sample;
        ring->prepared++;
    }
    checkpoint_save(ring);
    return true;
}
bool wf_oki_ring_commit(wf_oki_ring_t *ring, uint32_t played)
{
    if (!ring || !ring->ready || played < ring->played || played > ring->prepared)
        return false;
    ring->played = played;
    return true;
}
bool wf_oki_ring_bank(wf_oki_ring_t *ring, const wf_oki_map_t *map, int16_t *pcm,
                      unsigned bank, wf_oki_work_t *work)
{
    if (!ring || !ring->ready || !pcm || !work || bank > 1u ||
        ring->prepared < ring->played ||
        ring->prepared - ring->played > WF_OKI_RING) return false;
    *work = (wf_oki_work_t){0};
    if (bank == ring->bank) return true;
    uint32_t number = ring->played / WF_OKI_CHECKPOINT_STRIDE;
    uint32_t position = number * WF_OKI_CHECKPOINT_STRIDE;
    wf_oki_checkpoint_t restored;
    if (position >= ring->anchor_position) {
        restored = ring->checkpoints[number % WF_OKI_CHECKPOINTS];
        if (restored.tag != (uint8_t)number) return false;
    } else {
        position = ring->anchor_position;
        restored = ring->anchor;
    }
    if (position > ring->played || ring->played - position >= WF_OKI_CHECKPOINT_STRIDE)
        return false;
    uint32_t replay = ring->played - position, target = ring->prepared;
    /* The selected checkpoint is at or after the last committed bank write.
     * Therefore every nibble up to the new boundary uses the current old
     * bank. A new exact anchor avoids an unbounded bank-event history. */
    while (position < ring->played) {
        int16_t ignored;
        if (!wf_oki_decode_at(map, &ring->cache, ring->bank, ring->start,
                             ring->nibbles, position++, &restored, &ignored))
            return false;
    }
    ring->anchor = restored;
    ring->anchor_position = ring->played;
    ring->state = restored;
    ring->bank = (uint8_t)bank;
    ring->cache.valid = 0;
    ring->prepared = ring->played;
    if (!wf_oki_ring_fill(ring, map, pcm, target)) return false;
    work->restore_samples = replay;
    work->repair_samples = target - ring->played;
    return true;
}
