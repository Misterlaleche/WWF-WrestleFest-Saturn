/* Compile with the actual SH-2 compiler; the analyzer reads these constants
 * from ELF data. Host sizeof()/ctypes layouts are not evidence of target ABI. */
#include <stddef.h>
#include <stdint.h>
#include "wf_runtime.h"
const uint32_t wf_capture_layout[] __attribute__((section(".wf_capture_abi"),used)) = {
    0x57464c31u,
    sizeof(void *), sizeof(wf_runtime_t), sizeof(wf_globals_raw_t),
    offsetof(wf_runtime_t,actors), sizeof(((wf_runtime_t *)0)->actors),
    sizeof(wf_actor_raw_t), WF_ACTOR_SLOTS,
    offsetof(wf_runtime_t,g)+offsetof(wf_globals_raw_t,frame_counter),
    offsetof(wf_runtime_t,g)+offsetof(wf_globals_raw_t,mode_0161),
    offsetof(wf_runtime_t,frame_stage), offsetof(wf_runtime_t,frame_ready)
};
