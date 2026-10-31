#include "wf_prematch.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

typedef struct { unsigned calls; uint32_t digest; } entropy_log;
static uint32_t entropy(void *context, unsigned actor, unsigned update)
{
    entropy_log *log = context;
    uint32_t result = (uint32_t)(actor * 101u + update * 17u + log->calls++);
    log->digest = log->digest * 33u + result;
    return result;
}
static void same_entry(const wp01_scene_t *a, const wp01_scene_t *b)
{
    /* Hook contexts differ deliberately between the two real executions. */
    wp01_scene_t left = *a, right = *b;
    left.rng_context = right.rng_context = NULL;
    assert(memcmp(&left, &right, sizeof(left)) == 0);
}
static void inert(wf_prematch *p)
{
    wf_prematch saved = *p;
    unsigned i;
    for (i = 0; i < 9; ++i) assert(wf_prematch_step(p) == WF_PREMATCH_OUTPUT_NONE);
    assert(memcmp(&saved, p, sizeof(saved)) == 0);
}
/* Catches skipped setup/core outputs, early done, physical ID forwarding,
 * lost sprite latency, RNG context substitution, and dropped sound events. */
static void differential(const uint8_t ids[4], uint32_t seed)
{
    wf_prematch p;
    wp01_scene_t direct, previous = {0};
    wp02_scene presenter;
    entropy_log actual_rng = {0}, direct_rng = {0};
    wf_prematch_presenter_rng rng = {seed ^ 0xa5a5a5a5u, 0x12345678u,
                                    {1u, 3u, 5u, 7u, 11u, 13u, 17u}};
    unsigned i, visible = 0, events = 0;
    assert(wf_prematch_begin(&p, ids, seed, entropy, &actual_rng));
    assert(p.phase == WF_PREMATCH_LOAD_ENTRADA);
    assert(memcmp(p.identities, ids, 4) == 0);
    inert(&p);
    assert(wf_prematch_load_entrada(&p, 1));
    wp01_scene_init(&direct, ids[0], ids[1], seed);
    wp01_scene_set_rng_hook(&direct, entropy, &direct_rng);
    for (i = 0; i < 6; ++i) {
        assert(wf_prematch_step(&p) == WF_PREMATCH_OUTPUT_SETUP);
        same_entry(&p.entrada, &direct);
        same_entry(&p.entrada_previous, &previous);
        assert(p.entrada_outputs == 0 && !p.entrada.done);
    }
    for (i = 0; i < 402; ++i) {
        if (i) { previous = direct; wp01_scene_step(&direct); }
        assert(wf_prematch_step(&p) == WF_PREMATCH_OUTPUT_ENTRADA);
        assert(p.phase == WF_PREMATCH_ENTRADA && !p.entrada.done);
        assert(p.entrada_outputs == i + 1);
        same_entry(&p.entrada, &direct);
        same_entry(&p.entrada_previous, &previous);
        assert(actual_rng.calls == direct_rng.calls);
        assert(actual_rng.digest == direct_rng.digest);
    }
    assert(p.entrada.frame == 401);
    wp01_scene_step(&direct);
    assert(wf_prematch_step(&p) == WF_PREMATCH_OUTPUT_NONE);
    assert(p.phase == WF_PREMATCH_LOAD_PRESENTADORA);
    assert(p.entrada.done && p.entrada_outputs == 402);
    same_entry(&p.entrada, &direct);
    inert(&p);
    assert(wf_prematch_load_presentadora(&p, 1, &rng));
    assert(wp02_init(&presenter, ids, rng.seed));
    presenter.rng_global80 = rng.global80;
    memcpy(presenter.rng_mix, rng.mix, sizeof(rng.mix));
    assert(memcmp(&p.presentadora, &presenter, sizeof(presenter)) == 0);
    for (i = 0; i < 4; ++i) assert(p.presentadora.wrestlers[i].type == ids[i]);
    while (wp02_tick(&presenter)) {
        assert(wf_prematch_step(&p) == WF_PREMATCH_OUTPUT_PRESENTADORA);
        assert(p.phase == WF_PREMATCH_PRESENTADORA);
        assert(memcmp(&p.presentadora, &presenter, sizeof(presenter)) == 0);
        ++visible;
        events += p.presentadora.sound_count;
        for (i = 0; i < p.presentadora.sound_count; ++i)
            assert(p.presentadora.effective_commands[i] ==
                   (p.presentadora.raw_commands[i] | 0x3100u));
        assert(visible < 10000);
    }
    assert(visible > 0 && events > 0 && !p.presentadora.done);
    assert(p.presentadora.terminal_pending);
    assert(wf_prematch_step(&p) == WF_PREMATCH_OUTPUT_NONE);
    assert(p.phase == WF_PREMATCH_DONE && p.presentadora.done);
    assert(p.presentadora.ticks == visible);
    assert(p.presentadora_outputs == visible);
    assert(memcmp(&p.presentadora, &presenter, sizeof(presenter)) == 0);
    assert(p.presentadora.presenter.active == 0);
    inert(&p);
}
/* Catches permissive fallback selections and null callbacks/contexts. */
static void invalid_inputs(void)
{
    const uint8_t valid[4] = {0, 7, 10, 11};
    uint8_t bad[4];
    wf_prematch p = {0}, saved;
    entropy_log rng = {0};
    wf_prematch_presenter_rng context = {0};
    unsigned i, j;
    inert(&p);
    assert(!wf_prematch_begin(NULL, valid, 0, entropy, &rng));
    assert(!wf_prematch_begin(&p, NULL, 0, entropy, &rng));
    assert(p.phase == WF_PREMATCH_ERROR && p.error == WF_PREMATCH_INVALID);
    inert(&p);
    assert(!wf_prematch_begin(&p, valid, 0, NULL, &rng));
    assert(p.phase == WF_PREMATCH_ERROR && p.error == WF_PREMATCH_INVALID);
    for (i = 0; i < 4; ++i) {
        memcpy(bad, valid, 4); bad[i] = (uint8_t)(i & 1 ? 255 : 12);
        assert(!wf_prematch_begin(&p, bad, 0, entropy, &rng));
        assert(p.phase == WF_PREMATCH_ERROR); inert(&p);
        for (j = 0; j < i; ++j) {
            memcpy(bad, valid, 4); bad[i] = bad[j];
            assert(!wf_prematch_begin(&p, bad, 0, entropy, &rng));
            assert(p.phase == WF_PREMATCH_ERROR); inert(&p);
        }
    }
    assert(wf_prematch_step(NULL) == WF_PREMATCH_OUTPUT_NONE);
    assert(!wf_prematch_load_entrada(NULL, 1));
    assert(!wf_prematch_load_presentadora(NULL, 1, &context));
    /* A null opaque hook context is legal: callback ownership is external. */
    assert(wf_prematch_begin(&p, valid, 0, entropy, NULL));
    saved = p;
    assert(!wf_prematch_load_presentadora(&p, 1, &context));
    assert(memcmp(&saved, &p, sizeof(p)) == 0);
    assert(!wf_prematch_load_entrada(&p, 0));
    assert(p.phase == WF_PREMATCH_ERROR && p.error == WF_PREMATCH_LOAD_FAILED);
    inert(&p);
}
/* Catches silently advancing failed/missing presenter loads and re-entry. */
static void failed_presenter_load(void)
{
    const uint8_t ids[4] = {9, 0, 10, 11};
    entropy_log rng = {0};
    wf_prematch p, saved;
    unsigned i, attempt;
    for (attempt = 0; attempt < 2; ++attempt) {
        assert(wf_prematch_begin(&p, ids, 1, entropy, &rng));
        assert(wf_prematch_load_entrada(&p, 1));
        saved = p;
        assert(!wf_prematch_load_entrada(&p, 1));
        assert(memcmp(&saved, &p, sizeof(p)) == 0);
        for (i = 0; i < 409; ++i) (void)wf_prematch_step(&p);
        assert(p.phase == WF_PREMATCH_LOAD_PRESENTADORA);
        assert(!wf_prematch_load_presentadora(&p, attempt != 0, NULL));
        assert(p.phase == WF_PREMATCH_ERROR);
        assert(p.error == (attempt ? WF_PREMATCH_INVALID : WF_PREMATCH_LOAD_FAILED));
        assert(p.presentadora.ticks == 0 && p.entrada_outputs == 402);
        inert(&p);
    }
}
int main(void)
{
    unsigned id, slot, j;
    invalid_inputs();
    failed_presenter_load();
    /* Every raw identity in every logical slot; no selection policy inferred. */
    for (id = 0; id < 12; ++id) for (slot = 0; slot < 4; ++slot) {
        uint8_t ids[4];
        for (j = 0; j < 4; ++j) ids[j] = (uint8_t)((id + 1 + j) % 12);
        ids[slot] = (uint8_t)id;
        differential(ids, (uint32_t)(0x12345678u + id * 41 + slot));
    }
    {
        const uint8_t pairs[][4] = {{0, 9, 10, 11}, {9, 0, 11, 10},
                                  {10, 11, 0, 7}, {7, 0, 10, 11},
                                  {11, 10, 6, 2}};
        for (j = 0; j < sizeof(pairs) / sizeof(pairs[0]); ++j)
            differential(pairs[j], 0);
    }
    puts("prematch: 53 differential journeys, validation/load/terminal checks passed");
    return 0;
}
