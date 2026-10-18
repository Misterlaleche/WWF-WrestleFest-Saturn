#include <stddef.h>
#include <string.h>
#include "wf_runtime.h"
#include "wf_gameplay_bindings.h"

/*
 * Saturn program entrypoint for link validation and future hardware harness.
 *
 * This intentionally does NOT synthesize a direct-match bootstrap. The exact
 * actor/global initialization for entering a live match is still an explicit
 * gameplay dependency. Until an audited bootstrap owner supplies that state,
 * main() exits through a fail-closed status instead of fabricating fighters,
 * RNG, mode globals, or input samples merely to make a build run.
 */
int
main(void)
{
    static wf_runtime_t runtime;
    static wf_gameplay_binding_context_t binding_ctx;
    static wf_phase_binding_t bindings[40];

    memset(&runtime, 0, sizeof(runtime));
    const size_t binding_count =
        wf_gameplay_bindings_build(&binding_ctx, bindings,
                                   sizeof(bindings) / sizeof(bindings[0]));

    if (binding_count == 0u) {
        return (int)WF_BAD_STATE;
    }
    wf_runtime_init(&runtime, bindings, binding_count);

    /* SceneFlow/Prematch supplies the CLOSED handoff.  Standalone link harness
       has no such external state and therefore remains fail-closed. */
    return (int)WF_BLOCKED;
}
