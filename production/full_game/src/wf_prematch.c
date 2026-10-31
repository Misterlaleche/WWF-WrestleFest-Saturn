#include "wf_prematch.h"
#include <string.h>

static int fail(wf_prematch *p, wf_prematch_error error)
{
    p->error = error;
    p->phase = WF_PREMATCH_ERROR;
    return 0;
}

int wf_prematch_begin(wf_prematch *p, const uint8_t identities[4],
                     uint32_t entrada_seed, wp01_scene_rng_hook_t hook,
                     void *hook_context)
{
    uint8_t ids[4];
    unsigned i, j;
    if (!p) return 0;
    /* Copy first: restarting with p->identities is supported. */
    if (identities) memcpy(ids, identities, sizeof(ids));
    memset(p, 0, sizeof(*p));
    if (!identities || !hook) return fail(p, WF_PREMATCH_INVALID);
    for (i = 0; i < 4; ++i) {
        if (ids[i] >= 12) return fail(p, WF_PREMATCH_INVALID);
        for (j = 0; j < i; ++j)
            if (ids[i] == ids[j]) return fail(p, WF_PREMATCH_INVALID);
    }
    memcpy(p->identities, ids, sizeof(ids));
    wp01_scene_init(&p->entrada, ids[0], ids[1], entrada_seed);
    wp01_scene_set_rng_hook(&p->entrada, hook, hook_context);
    p->phase = WF_PREMATCH_LOAD_ENTRADA;
    return 1;
}

int wf_prematch_load_entrada(wf_prematch *p, int success)
{
    if (!p || p->phase != WF_PREMATCH_LOAD_ENTRADA) return 0;
    if (!success) return fail(p, WF_PREMATCH_LOAD_FAILED);
    p->phase = WF_PREMATCH_ENTRADA;
    return 1;
}

int wf_prematch_load_presentadora(wf_prematch *p, int success,
                                const wf_prematch_presenter_rng *rng)
{
    if (!p || p->phase != WF_PREMATCH_LOAD_PRESENTADORA) return 0;
    if (!success) return fail(p, WF_PREMATCH_LOAD_FAILED);
    if (!rng) return fail(p, WF_PREMATCH_INVALID);
    if (!wp02_init(&p->presentadora, p->identities, rng->seed))
        return fail(p, WF_PREMATCH_CORE_ERROR);
    p->presentadora.rng_global80 = rng->global80;
    memcpy(p->presentadora.rng_mix, rng->mix, sizeof(rng->mix));
    p->phase = WF_PREMATCH_PRESENTADORA;
    return 1;
}

wf_prematch_output wf_prematch_step(wf_prematch *p)
{
    if (!p) return WF_PREMATCH_OUTPUT_NONE;
    if (p->phase == WF_PREMATCH_ENTRADA) {
        if (p->setup_outputs < 6) {
            ++p->setup_outputs;
            return WF_PREMATCH_OUTPUT_SETUP;
        }
        if (p->entrada_outputs) {
            p->entrada_previous = p->entrada;
            wp01_scene_step(&p->entrada);
        }
        if (p->entrada.done) {
            p->phase = WF_PREMATCH_LOAD_PRESENTADORA;
            return WF_PREMATCH_OUTPUT_NONE;
        }
        ++p->entrada_outputs;
        return WF_PREMATCH_OUTPUT_ENTRADA;
    }
    if (p->phase == WF_PREMATCH_PRESENTADORA) {
        if (wp02_tick(&p->presentadora)) {
            ++p->presentadora_outputs;
            return WF_PREMATCH_OUTPUT_PRESENTADORA;
        }
        if (p->presentadora.error) (void)fail(p, WF_PREMATCH_CORE_ERROR);
        else if (p->presentadora.done) p->phase = WF_PREMATCH_DONE;
    }
    return WF_PREMATCH_OUTPUT_NONE;
}
