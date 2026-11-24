/* Host-only runner for the unchanged target phase and its facing consumer.
 * Including the prepass makes its private $10BE8 entry callable without
 * adding a test API to production. Unused prepass paths are linker-collected.
 */
#include <stdio.h>
#include <string.h>
#include "wf_target_relation.h"
#include "../core/src/wf_actor_prepass.c"

static wf_runtime_t runtime;

int main(void)
{
    unsigned char header[4];
    while (fread(header, 1, sizeof header, stdin) == sizeof header) {
        memset(&runtime, 0, sizeof runtime);
        if (fread(runtime.actors, 1, sizeof runtime.actors, stdin) != sizeof runtime.actors)
            return 2;
        if (header[1] >= WF_ACTOR_SLOTS)
            return 2;
        runtime.g.mode_0161 = header[0];
        runtime.m68k_regs.valid = 1;
        wf_status_t selection = WF_OK;
        if (!header[2])
            selection = wf_phase_target_relation_20958(&runtime, 0x20958u, NULL);
        wf_status_t facing = facing_10be8(&runtime, &runtime.actors[header[1]], NULL);
        unsigned char status[2] = {(unsigned char)selection, (unsigned char)facing};
        if (fwrite(status, 1, sizeof status, stdout) != sizeof status ||
            fwrite(runtime.actors, 1, sizeof runtime.actors, stdout) != sizeof runtime.actors)
            return 2;
    }
    return ferror(stdin) || ferror(stdout) ? 2 : 0;
}
