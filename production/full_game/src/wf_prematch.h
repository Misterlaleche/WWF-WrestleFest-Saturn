#ifndef WF_PREMATCH_H
#define WF_PREMATCH_H
#include <stdint.h>
#include "wp01_scene.h"
#include "wp02_scene.h"

typedef enum {
    WF_PREMATCH_IDLE, WF_PREMATCH_LOAD_ENTRADA, WF_PREMATCH_ENTRADA,
    WF_PREMATCH_LOAD_PRESENTADORA, WF_PREMATCH_PRESENTADORA,
    WF_PREMATCH_DONE, WF_PREMATCH_ERROR
} wf_prematch_phase;
typedef enum {
    WF_PREMATCH_OK, WF_PREMATCH_INVALID, WF_PREMATCH_LOAD_FAILED,
    WF_PREMATCH_CORE_ERROR
} wf_prematch_error;
typedef enum {
    WF_PREMATCH_OUTPUT_NONE, WF_PREMATCH_OUTPUT_SETUP,
    WF_PREMATCH_OUTPUT_ENTRADA, WF_PREMATCH_OUTPUT_PRESENTADORA
} wf_prematch_output;
/* Required independent presenter context, copied at successful load.
 * It is not derived from Entrada RNG state and is not a cold-boot policy.
 * The intact wp02 core can update mix fields as its composer requires. */
typedef struct {
    uint32_t seed, global80, mix[7];
} wf_prematch_presenter_rng;
/* Caller-owned object: treat all fields as read-only between API calls.
 * Snapshot lifetimes end at the next begin/load/step. During an ENTRADA
 * output, backgrounds use entrada and buffered sprites use entrada_previous.
 * The previous snapshot is zero for the first core output (frame 0).
 * Presenter sound_count selects this output's raw/effective_commands;
 * observe/copy them before the next call. No audio backend is linked. */
typedef struct {
    wf_prematch_phase phase;
    wf_prematch_error error;
    uint8_t identities[4];
    unsigned setup_outputs, entrada_outputs, presentadora_outputs;
    wp01_scene_t entrada, entrada_previous;
    wp02_scene presentadora;
} wf_prematch;

/* Validates four distinct raw ROM IDs <12; no selection/pair policy.
 * Entrada receives original slots 0,1; its physical ordering never changes
 * identities, which are forwarded intact to wp02. A non-null RNG hook is
 * mandatory; its opaque context may be null if the hook supports it.
 * Reinitializes the object, including after DONE/ERROR. No RNG call, scene
 * tick or resource I/O occurs here. The explicit seed belongs to Entrada.
 * Null object returns 0; invalid data puts a non-null object in ERROR. */
int wf_prematch_begin(wf_prematch *p, const uint8_t identities[4],
                     uint32_t entrada_seed, wp01_scene_rng_hook_t hook,
                     void *hook_context);
/* Explicit owner confirmation of resource readiness. success==0 enters
 * ERROR/LOAD_FAILED; failed loads never emit outputs. Out-of-phase calls
 * return 0 without mutation. Loading is a barrier and step is inert there.
 * This adapter does not read or validate resource files itself. */
int wf_prematch_load_entrada(wf_prematch *p, int success);
/* Requires LOAD_PRESENTADORA and, on success, a non-null RNG context.
 * No setup/display update is consumed by initialization. A null context
 * with success!=0 enters ERROR/INVALID; failure ignores the context. */
int wf_prematch_load_presentadora(wf_prematch *p, int success,
                                const wf_prematch_presenter_rng *rng);
/* One owner-supplied arcade update request, with no private clock.
 * Emits six SETUP outputs, then 402 ENTRADA outputs for core frames 0..401.
 * The next call performs the core's terminal step and enters presenter
 * loading, returning NONE. Thus done is never set before frame401 emits.
 * wp02 emits exactly the updates counted by its ticks field. Its terminal
 * clear returns NONE and enters DONE without incrementing that count.
 * NONE requires no rendering and accounts for no extra visible core tick.
 * IDLE, loading, DONE, ERROR and null input are inert. There is no automatic
 * progression into a fight and no synthesis, skipping or catch-up of ticks. */
wf_prematch_output wf_prematch_step(wf_prematch *p);
#endif
