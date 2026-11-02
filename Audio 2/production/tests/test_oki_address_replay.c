/* Private ROM bytes are loaded at run time, never embedded in this test. */
#include "oki_address_replay/wf_oki_address.h"
#include "../tools/reference/reference_oki.h"
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define REQUIRE(expr) do { if (!(expr)) { \
    fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #expr); \
    return 1; } } while (0)

static unsigned be24(const uint8_t *p)
{ return (unsigned)p[0] * 65536u + (unsigned)p[1] * 256u + p[2]; }

static uint8_t *load(const char *name, size_t *bytes)
{
    FILE *f = fopen(name, "rb");
    if (!f || fseek(f, 0, SEEK_END)) return NULL;
    long length = ftell(f);
    if (length < 0 || fseek(f, 0, SEEK_SET)) { fclose(f); return NULL; }
    *bytes = (size_t)length;
    uint8_t *data = malloc(*bytes);
    if (!data || fread(data, 1, *bytes, f) != *bytes) {
        free(data); fclose(f); return NULL;
    }
    fclose(f);
    return data;
}

/* Catches endian, inclusive-end, alias, cross-bank, missing-range and stale
 * span-cache errors against the original full ROM, independent of the map. */
static int resolver_test(const uint8_t *rom, const uint8_t *pack, size_t pack_bytes,
                         const uint8_t *extension, size_t extension_bytes)
{
    wf_oki_map_t map;
    wf_oki_span_t cache = {0};
    uint8_t *required = calloc(WF_OKI_WINDOW, 1);
    uint64_t checked = 0, masked = 0, missing = 0;
    unsigned valid[2] = {0, 0};
    REQUIRE(required);
    REQUIRE(wf_oki_map_init(&map, pack, pack_bytes, extension, extension_bytes));
    for (unsigned bank = 0; bank < 2; bank++) {
        for (unsigned id = 0; id < 128; id++) {
            const uint8_t *table = rom + bank * WF_OKI_WINDOW + id * 8;
            unsigned first = be24(table) & (WF_OKI_WINDOW - 1);
            unsigned last = be24(table + 3) & (WF_OKI_WINDOW - 1);
            uint32_t start, nibbles;
            if (first >= last) {
                REQUIRE(!wf_oki_map_sample(&map, bank, id, &start, &nibbles));
                continue;
            }
            valid[bank]++;
            REQUIRE(wf_oki_map_sample(&map, bank, id, &start, &nibbles));
            REQUIRE(start == first && nibbles == 2u * (last - first + 1u));
            memset(required + first, 1, last - first + 1u);
        }
    }
    REQUIRE(valid[0] == 73 && valid[1] == 41);
    /* Alternate bank on every address: cached ranges may never retain a
     * start-latched bank, even where both banks have overlapping ranges. */
    for (unsigned address = 0; address < WF_OKI_WINDOW; address++) {
        for (unsigned bank = 0; bank < 2; bank++) {
            uint8_t value = 0;
            bool found = wf_oki_map_read(&map, &cache, bank, address, &value);
            REQUIRE(found == (required[address] != 0));
            if (!found) continue;
            REQUIRE(value == rom[bank * WF_OKI_WINDOW + address]);
            checked++;
            if (bank == 0 && address >= 0x3f554 && address <= 0x3f6a4) missing++;
            if (!(address & 255u)) {
                REQUIRE(wf_oki_map_read(&map, &cache, bank,
                                       address | 0xfffc0000u, &value));
                REQUIRE(value == rom[bank * WF_OKI_WINDOW + address]);
                masked++;
            }
        }
    }
    REQUIRE(checked == 517450 && missing == 337);
    uint8_t value;
    REQUIRE(!wf_oki_map_read(&map, &cache, 2, 0x3b1b2, &value));
    /* A truncated or malformed metadata span must not become a read outside
     * the source arrays. No claim of a production resource validator is made. */
    REQUIRE(!wf_oki_map_init(&map, pack, pack_bytes, extension, 15));
    REQUIRE(!wf_oki_map_init(&map, pack, pack_bytes, extension, extension_bytes - 1));
    uint8_t *bad = malloc(extension_bytes);
    REQUIRE(bad);
    const unsigned corrupt_at[] = {0, 5, 7, 9, 11, 12, 16, 358, 361, 363};
    for (unsigned i = 0; i < sizeof corrupt_at / sizeof *corrupt_at; i++) {
        memcpy(bad, extension, extension_bytes);
        bad[corrupt_at[i]] = 0xff;
        REQUIRE(!wf_oki_map_init(&map, pack, pack_bytes, bad, extension_bytes));
    }
    free(bad); free(required);
    printf("{\"mode\":\"resolver\",\"passed\":true,\"physical_bytes\":%" PRIu64
           ",\"missing_payload_bytes\":%" PRIu64 ",\"masked_reads\":%" PRIu64 "}\n",
           checked, missing, masked);
    return 0;
}

/* Catches nibble parity, global-bank retention, signal/index resets, clipping,
 * inclusive sample ends and silent-tail changes against the independent model. */
static int decoder_test(const uint8_t *rom, const uint8_t *pack, size_t pack_bytes,
                        const uint8_t *extension, size_t extension_bytes)
{
    wf_oki_map_t map;
    uint64_t decoded = 0, changes[2] = {0, 0}, clipped = 0, tails = 0;
    REQUIRE(wf_oki_map_init(&map, pack, pack_bytes, extension, extension_bytes));
    for (unsigned initial_bank = 0; initial_bank < 2; initial_bank++) {
        for (unsigned id = 1; id <= (initial_bank ? 41u : 73u); id++) {
            uint32_t start, nibbles;
            REQUIRE(wf_oki_map_sample(&map, initial_bank, id, &start, &nibbles));
            for (unsigned scenario = 0; scenario < 4; scenario++) {
                wf_oki_span_t cache = {0};
                wf_oki_checkpoint_t state = {0, 0, 0x5a};
                reference_oki_t original;
                reference_oki_reset(&original, rom, 2u * WF_OKI_WINDOW);
                reference_oki_bank(&original, initial_bank);
                reference_oki_write(&original, 0x80u | id);
                reference_oki_write(&original, 0x10u);
                REQUIRE((reference_oki_status(&original) & 15u) == 1u);
                unsigned bank = initial_bank;
                for (uint32_t position = 0; position < nibbles + 3u; position++) {
                    bool change = (scenario == 1 && position == 1u) ||
                                  (scenario == 2 && position == 2u) ||
                                  (scenario == 3 && (position < 4u ||
                                    position % 127u == 0u || position % 128u < 2u ||
                                    position + 2u >= nibbles));
                    if (change) {
                        bank ^= 1u;
                        reference_oki_bank(&original, bank);
                        changes[position & 1u]++;
                    }
                    int16_t pcm = 123;
                    REQUIRE(wf_oki_decode_at(&map, &cache, bank, start, nibbles,
                                             position, &state, &pcm));
                    reference_oki_tick(&original);
                    REQUIRE(pcm == original.voices[0].current);
                    REQUIRE(state.signal == original.voices[0].signal);
                    REQUIRE(state.index == original.voices[0].index && state.tag == 0x5a);
                    REQUIRE(reference_oki_mix(&original) == pcm);
                    decoded++;
                    if (state.signal == 2047 || state.signal == -2048) clipped++;
                    if (position >= nibbles) { REQUIRE(pcm == 0); tails++; }
                }
                REQUIRE(!original.fault);
            }
        }
    }
    REQUIRE(changes[0] && changes[1] && clipped && tails == 114u * 4u * 3u);
    wf_oki_span_t cache = {0};
    wf_oki_checkpoint_t bad = {0, 49, 0};
    int16_t pcm;
    REQUIRE(!wf_oki_decode_at(&map, &cache, 0, 0x3b1b2, 2, 0, &bad, &pcm));
    printf("{\"mode\":\"decoder\",\"passed\":true,\"samples_compared\":%" PRIu64
           ",\"bank_changes_at_even_nibble\":%" PRIu64
           ",\"bank_changes_at_odd_nibble\":%" PRIu64
           ",\"clipped_states\":%" PRIu64 ",\"zero_tail_samples\":%" PRIu64 "}\n",
           decoded, changes[0], changes[1], clipped, tails);
    return 0;
}

typedef struct {
    uint64_t consumed, future_compared, bank_changes, repeated_same_cursor;
    uint32_t maximum_restore, maximum_repair;
} ring_counts_t;

/* Real ring repair is compared with a fresh continuation of the independent
 * decoder. Merely updating the bank field must fail this test. */
static int check_repair(wf_oki_ring_t *ring, const wf_oki_map_t *map,
                         int16_t *pcm, reference_oki_t *original,
                         unsigned voice, ring_counts_t *counts)
{
    int16_t before[WF_OKI_RING + 1];
    memcpy(before, pcm, sizeof before);
    unsigned bank = original->bank ^ 1u;
    reference_oki_bank(original, bank);
    wf_oki_work_t work = {9999, 9999};
    REQUIRE(wf_oki_ring_bank(ring, map, pcm, bank, &work));
    REQUIRE(work.restore_samples < WF_OKI_CHECKPOINT_STRIDE);
    REQUIRE(work.repair_samples <= WF_OKI_RING);
    REQUIRE(ring->anchor_position == ring->played);
    REQUIRE(ring->anchor.signal == original->voices[voice].signal);
    REQUIRE(ring->anchor.index == original->voices[voice].index);
    if (work.restore_samples > counts->maximum_restore)
        counts->maximum_restore = work.restore_samples;
    if (work.repair_samples > counts->maximum_repair)
        counts->maximum_repair = work.repair_samples;
    counts->bank_changes++;
    reference_oki_t future = *original;
    for (uint32_t pos = ring->played; pos < ring->prepared; pos++) {
        reference_oki_tick(&future);
        REQUIRE(pcm[pos % WF_OKI_RING] == future.voices[voice].current);
        counts->future_compared++;
    }
    for (uint32_t pos = ring->prepared; pos < ring->played + WF_OKI_RING; pos++)
        REQUIRE(pcm[pos % WF_OKI_RING] == before[pos % WF_OKI_RING]);
    REQUIRE(pcm[WF_OKI_RING] == pcm[0]);
    REQUIRE(wf_oki_ring_bank(ring, map, pcm, bank, &work));
    REQUIRE(work.restore_samples == 0 && work.repair_samples == 0);
    return 0;
}

static int ring_test(const uint8_t *rom, const uint8_t *pack, size_t pack_bytes,
                     const uint8_t *extension, size_t extension_bytes)
{
    wf_oki_map_t map;
    ring_counts_t counts = {0};
    REQUIRE(wf_oki_map_init(&map, pack, pack_bytes, extension, extension_bytes));
    for (unsigned initial_bank = 0; initial_bank < 2; initial_bank++) {
        for (unsigned id = 1; id <= (initial_bank ? 41u : 73u); id++) {
            wf_oki_ring_t ring;
            int16_t pcm[WF_OKI_RING + 1];
            for (unsigned i = 0; i <= WF_OKI_RING; i++) pcm[i] = 12345;
            reference_oki_t original;
            reference_oki_reset(&original, rom, 2u * WF_OKI_WINDOW);
            reference_oki_bank(&original, initial_bank);
            reference_oki_write(&original, 0x80u | id);
            reference_oki_write(&original, 0x10u);
            REQUIRE(wf_oki_ring_start(&ring, &map, initial_bank, id));
            uint32_t end = ring.nibbles + 260u;
            uint32_t target = end < WF_OKI_RING ? end : WF_OKI_RING;
            REQUIRE(wf_oki_ring_fill(&ring, &map, pcm, target));
            REQUIRE(!wf_oki_ring_fill(&ring, &map, pcm, WF_OKI_RING + 1u));
            REQUIRE(!wf_oki_ring_commit(&ring, target + 1u));
            for (uint32_t pos = 0; pos < end; pos++) {
                const uint32_t events[] = {0,1,2,63,64,65,126,127,128,129,130,131,
                    255,256,257,511,4094,4095,4096,4097,8191,8192,8193,
                    ring.nibbles - 2u, ring.nibbles - 1u, ring.nibbles,
                    ring.nibbles + 1u, ring.nibbles + 127u, ring.nibbles + 128u};
                bool change = pos % WF_OKI_RING < 2u || pos % WF_OKI_RING >= WF_OKI_RING - 2u;
                for (unsigned e = 0; e < sizeof events / sizeof *events; e++)
                    if (pos == events[e]) change = true;
                if (change) {
                    REQUIRE(check_repair(&ring, &map, pcm, &original, 0, &counts) == 0);
                    if (pos == 1u || pos == 127u || pos == 128u || pos == 4095u ||
                        pos == 4096u || pos + 1u == ring.nibbles) {
                        REQUIRE(check_repair(&ring, &map, pcm, &original, 0, &counts) == 0);
                        counts.repeated_same_cursor++;
                    }
                }
                reference_oki_tick(&original);
                REQUIRE(pcm[pos % WF_OKI_RING] == original.voices[0].current);
                REQUIRE(pcm[WF_OKI_RING] == pcm[0]);
                counts.consumed++;
                REQUIRE(wf_oki_ring_commit(&ring, pos + 1u));
                target = end - (pos + 1u) < WF_OKI_RING ? end : pos + 1u + WF_OKI_RING;
                REQUIRE(wf_oki_ring_fill(&ring, &map, pcm, target));
            }
            wf_oki_work_t work;
            REQUIRE(!wf_oki_ring_bank(&ring, &map, pcm, 2, &work));
            REQUIRE(!wf_oki_ring_commit(&ring, 0));
        }
    }
    REQUIRE(counts.maximum_restore == 127 && counts.maximum_repair == 4096);
    REQUIRE(counts.repeated_same_cursor > 0);
    printf("{\"mode\":\"ring\",\"passed\":true,\"consumed_samples\":%" PRIu64
           ",\"prepared_samples_compared\":%" PRIu64 ",\"bank_changes\":%" PRIu64
           ",\"repeated_changes_at_same_cursor\":%" PRIu64
           ",\"maximum_restore_samples\":%u,\"maximum_repair_samples\":%u}\n",
           counts.consumed, counts.future_compared, counts.bank_changes,
           counts.repeated_same_cursor, counts.maximum_restore, counts.maximum_repair);
    return 0;
}

enum { BOARD_CLOCK = 3579545 };
typedef struct {
    reference_oki_t original, latched[4];
    wf_oki_ring_t voices[4];
    int16_t pcm[4][WF_OKI_RING + 1], current[4];
    uint64_t ticks, frames, origin, changed_frames, first_frame, last_frame;
    uint64_t bank_mismatch[4], byte_changes[4], nibble_changes[4], pcm_changes[4];
    uint64_t compared, repairs, repair_samples;
    uint32_t first_address[4], maximum_restore, maximum_repair, maximum_all_voice_work;
    unsigned commands, starts, logged_starts, capturing;
} trace_state_t;

static int trace_tick(trace_state_t *t, const wf_oki_map_t *map)
{
    unsigned active = reference_oki_status(&t->original) & 15u;
    for (unsigned v = 0; v < 4; v++) {
        if (!(active & (1u << v))) continue;
        const reference_oki_voice_t *p = &t->original.voices[v];
        unsigned address = (p->start + p->position / 2u) & (WF_OKI_WINDOW - 1u);
        unsigned actual = t->original.rom[t->original.bank * WF_OKI_WINDOW + address];
        unsigned fixed = t->original.rom[t->latched[v].bank * WF_OKI_WINDOW + address];
        if (t->original.bank != t->latched[v].bank) t->bank_mismatch[v]++;
        if (actual != fixed) {
            if (!t->byte_changes[v]) t->first_address[v] = address;
            t->byte_changes[v]++;
        }
        if ((p->position & 1u ? actual & 15u : actual >> 4) !=
            (p->position & 1u ? fixed & 15u : fixed >> 4)) t->nibble_changes[v]++;
    }
    reference_oki_tick(&t->original);
    int mix = 0;
    for (unsigned v = 0; v < 4; v++) {
        reference_oki_tick(&t->latched[v]);
        if (active & (1u << v)) {
            wf_oki_ring_t *ring = &t->voices[v];
            t->current[v] = t->pcm[v][ring->played % WF_OKI_RING];
            REQUIRE(wf_oki_ring_commit(ring, ring->played + 1u));
            if (t->original.voices[v].playing)
                REQUIRE(wf_oki_ring_fill(ring, map, t->pcm[v], ring->played + WF_OKI_RING));
            else ring->ready = 0;
        } else t->current[v] = 0;
        REQUIRE(t->current[v] == t->original.voices[v].current);
        if (t->original.voices[v].current != t->latched[v].voices[0].current)
            t->pcm_changes[v]++;
        mix += t->current[v] * reference_oki_levels[t->original.voices[v].volume] / 32;
        t->compared++;
    }
    REQUIRE(mix == reference_oki_mix(&t->original));
    t->ticks++;
    return 0;
}

/* board_advance ticks OKI before the 44.1 kHz output at the same source clock.
 * A bus event at cycle C sees all ticks/output boundaries <= C. Integer
 * ceiling times reproduce that order without running the FM chip again. */
static int trace_advance(trace_state_t *t, const wf_oki_map_t *map, uint64_t until)
{
    for (;;) {
        uint64_t next_tick = ((t->ticks + 1u) * BOARD_CLOCK + 7999u) / 8000u;
        uint64_t next_frame = t->capturing ?
            t->origin + ((t->frames + 1u) * BOARD_CLOCK + 44099u) / 44100u : UINT64_MAX;
        if (next_tick <= next_frame && next_tick <= until) {
            REQUIRE(trace_tick(t, map) == 0);
        } else if (next_frame <= until) {
            int fixed = 0;
            for (unsigned v = 0; v < 4; v++) fixed += reference_oki_mix(&t->latched[v]);
            if (reference_oki_mix(&t->original) != fixed) {
                if (!t->changed_frames) t->first_frame = t->frames;
                t->last_frame = t->frames;
                t->changed_frames++;
            }
            t->frames++;
        } else break;
    }
    return 0;
}

static int trace_test(const uint8_t *rom, const uint8_t *pack, size_t pack_bytes,
                      const uint8_t *extension, size_t extension_bytes, const char *path)
{
    wf_oki_map_t map;
    REQUIRE(wf_oki_map_init(&map, pack, pack_bytes, extension, extension_bytes));
    trace_state_t *t = calloc(1, sizeof *t);
    REQUIRE(t);
    reference_oki_reset(&t->original, rom, 2u * WF_OKI_WINDOW);
    for (unsigned v = 0; v < 4; v++) reference_oki_reset(&t->latched[v], rom, 2u * WF_OKI_WINDOW);
    FILE *trace = fopen(path, "r");
    REQUIRE(trace);
    char line[256], kind[16];
    REQUIRE(fgets(line, sizeof line, trace));
    uint64_t previous = 0;
    unsigned ended = 0;
    while (fgets(line, sizeof line, trace)) {
        unsigned long long cycle;
        unsigned a, b;
        REQUIRE(sscanf(line, "%llu,%15[^,],%u,%u", &cycle, kind, &a, &b) == 4);
        REQUIRE(cycle >= previous && !ended);
        previous = cycle;
        REQUIRE(trace_advance(t, &map, cycle) == 0);
        if (!strcmp(kind, "BANK")) {
            unsigned old = t->original.bank;
            reference_oki_bank(&t->original, b);
            uint32_t event_work = 0;
            if (old != t->original.bank) for (unsigned v = 0; v < 4; v++) {
                if (!t->original.voices[v].playing) continue;
                wf_oki_work_t work;
                REQUIRE(wf_oki_ring_bank(&t->voices[v], &map, t->pcm[v], t->original.bank, &work));
                REQUIRE(t->voices[v].anchor.signal == t->original.voices[v].signal);
                REQUIRE(t->voices[v].anchor.index == t->original.voices[v].index);
                t->repairs++; t->repair_samples += work.repair_samples;
                event_work += work.restore_samples + work.repair_samples;
                if (work.restore_samples > t->maximum_restore) t->maximum_restore = work.restore_samples;
                if (work.repair_samples > t->maximum_repair) t->maximum_repair = work.repair_samples;
            }
            if (event_work > t->maximum_all_voice_work) t->maximum_all_voice_work = event_work;
        } else if (!strcmp(kind, "OKI")) {
            int pending = t->original.pending;
            unsigned before = reference_oki_status(&t->original) & 15u;
            reference_oki_write(&t->original, b);
            unsigned after = reference_oki_status(&t->original) & 15u;
            for (unsigned v = 0; v < 4; v++) {
                if (!(before & (1u << v)) && (after & (1u << v))) {
                    REQUIRE(pending >= 0);
                    REQUIRE(wf_oki_ring_start(&t->voices[v], &map, t->original.bank, (unsigned)pending));
                    REQUIRE(t->voices[v].start == t->original.voices[v].start);
                    REQUIRE(t->voices[v].nibbles == t->original.voices[v].count);
                    REQUIRE(wf_oki_ring_fill(&t->voices[v], &map, t->pcm[v], WF_OKI_RING));
                    reference_oki_reset(&t->latched[v], rom, 2u * WF_OKI_WINDOW);
                    t->latched[v].bank = t->original.bank;
                    t->latched[v].voices[0] = t->original.voices[v];
                    t->current[v] = 0;
                    t->starts++;
                } else if ((before & (1u << v)) && !(after & (1u << v))) {
                    t->voices[v].ready = 0;
                    t->latched[v].voices[0].playing = 0;
                    t->latched[v].voices[0].current = 0;
                    t->current[v] = 0;
                }
            }
        } else if (!strcmp(kind, "ORIGIN")) {
            REQUIRE(!t->capturing);
            t->capturing = 1; t->origin = cycle;
        } else if (!strcmp(kind, "CMD")) {
            REQUIRE(a == t->commands);
            t->commands++;
        } else if (!strcmp(kind, "PLAY")) t->logged_starts++;
        else if (!strcmp(kind, "END")) ended = 1;
    }
    fclose(trace);
    REQUIRE(ended && t->commands == 64 && t->starts == t->logged_starts);
    REQUIRE(!t->original.fault && !t->original.invalid_starts && !t->original.ignored_busy_starts);
    REQUIRE(t->original.bank_changes_while_active == 5);
    REQUIRE(t->bank_mismatch[0] == 3561 && t->bank_mismatch[1] == 2667);
    REQUIRE(t->byte_changes[1] == 2641 && t->nibble_changes[1] == 2406 && t->pcm_changes[1] == 2663);
    REQUIRE(t->first_address[1] == 0x3b1b2);
    REQUIRE(t->byte_changes[0] == 0 && t->pcm_changes[0] == 0 &&
            t->pcm_changes[2] == 0 && t->pcm_changes[3] == 0);
    REQUIRE(t->frames == 6085800 && t->changed_frames == 14680);
    REQUIRE(t->first_frame == 2451772 && t->last_frame == 2466473);
    printf("{\"mode\":\"trace\",\"passed\":true,\"commands\":%u,\"physical_starts\":%u,"
           "\"oki_ticks\":%" PRIu64 ",\"voice_outputs_compared\":%" PRIu64
           ",\"output_frames\":%" PRIu64 ",\"active_bank_events\":%" PRIu64
           ",\"voice_repairs\":%" PRIu64 ",\"repaired_samples\":%" PRIu64
           ",\"maximum_restore_samples\":%u,\"maximum_repair_samples\":%u,"
           "\"maximum_all_voice_work_samples\":%u,\"counterfactual_byte_changes\":%" PRIu64
           ",\"counterfactual_decoded_changes\":%" PRIu64 ",\"counterfactual_changed_frames\":%" PRIu64
           ",\"first_changed_frame\":%" PRIu64 ",\"last_changed_frame\":%" PRIu64 "}\n",
           t->commands, t->starts, t->ticks, t->compared, t->frames,
           t->original.bank_changes_while_active, t->repairs, t->repair_samples,
           t->maximum_restore, t->maximum_repair, t->maximum_all_voice_work,
           t->byte_changes[1], t->pcm_changes[1], t->changed_frames, t->first_frame, t->last_frame);
    free(t);
    return 0;
}

int main(int argc, char **argv)
{
    if (argc != 5 && argc != 6) {
        fprintf(stderr, "usage: %s MODE SAMPLES.ROM SFX.DAT bankmap-extension.bin [TRACE.csv]\n", argv[0]);
        return 2;
    }
    size_t rom_bytes, pack_bytes, extension_bytes;
    uint8_t *rom = load(argv[2], &rom_bytes), *pack = load(argv[3], &pack_bytes);
    uint8_t *extension = load(argv[4], &extension_bytes);
    REQUIRE(rom && pack && extension && rom_bytes == 2u * WF_OKI_WINDOW);
    int result;
    if (!strcmp(argv[1], "resolver"))
        result = resolver_test(rom, pack, pack_bytes, extension, extension_bytes);
    else if (!strcmp(argv[1], "decoder"))
        result = decoder_test(rom, pack, pack_bytes, extension, extension_bytes);
    else if (!strcmp(argv[1], "ring"))
        result = ring_test(rom, pack, pack_bytes, extension, extension_bytes);
    else if (!strcmp(argv[1], "trace") && argc == 6)
        result = trace_test(rom, pack, pack_bytes, extension, extension_bytes, argv[5]);
    else result = 2;
    free(rom); free(pack); free(extension);
    return result;
}
