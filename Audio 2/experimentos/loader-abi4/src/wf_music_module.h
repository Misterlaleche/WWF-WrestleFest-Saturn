#ifndef WF_MUSIC_MODULE_H
#define WF_MUSIC_MODULE_H
#include <stddef.h>
#include <stdint.h>
enum {
 WF_MUSIC_MODULE_BASE=0x05c5c000u, WF_MUSIC_MODULE_END=0x05c80000u,
 WF_MUSIC_MODULE_SHARED=0x25c7f000u, WF_MUSIC_MODULE_MAGIC=0x57464d53u,
 WF_MUSIC_MODULE_ABI=4, WF_MUSIC_HOT_WINDOW_BYTES=18432,
 WF_MUSIC_BOOTING=1, WF_MUSIC_RUNNING=2, WF_MUSIC_FAULT=3,
 WF_NATIVE_MUSIC_PROFILE=0x4d46u
};
typedef struct {
 volatile uint16_t producer,consumer,commands[64];
 volatile uint16_t state,shutdown;
 volatile uint32_t error,events,steps_hi,steps_lo,skipped_hi,skipped_lo;
 volatile uint16_t generation,last_command;
 volatile uint16_t master_frame,worker_frame,dsp_progress_frame,reserved;
 /* Linker-owned cached HWRAM address, supplied by the master. */
 volatile uint32_t workspace;
} wf_music_module_shared_t;
_Static_assert(sizeof(wf_music_module_shared_t)==176,"music shared ABI size changed");
_Static_assert(offsetof(wf_music_module_shared_t,workspace)==172,"music workspace ABI offset changed");
/* The 32-byte BE header ends with the master's exact HWRAM window base.
 * Four loads: VDP1 program, private BSS, shared page, HWRAM code/state.
 * Both entry functions remain inside the VDP1 executable segment. */
#endif
