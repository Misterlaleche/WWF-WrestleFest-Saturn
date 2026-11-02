#ifndef WF_MUSIC_MODULE_H
#define WF_MUSIC_MODULE_H
#include <stddef.h>
#include <stdint.h>
enum {
#if defined(WF_AUDIO2_REGION_VDP2) && WF_AUDIO2_REGION_VDP2
 /* Royal HW1 retains its full VDP1 texture pool. This separate profile owns
  * only the audited gap after VDP2 bank1's three pattern-name tables. */
 WF_MUSIC_MODULE_BASE=0x05e26000u, WF_MUSIC_MODULE_STATE=0x05e3b000u,
 WF_MUSIC_MODULE_SHARED=0x25e3e000u, WF_MUSIC_MODULE_END=0x05e3f000u,
#else
 WF_MUSIC_MODULE_BASE=0x05c5c000u, WF_MUSIC_MODULE_END=0x05c80000u,
 WF_MUSIC_MODULE_STATE=0x05c7c000u, WF_MUSIC_MODULE_SHARED=0x25c7f000u,
#endif
 WF_MUSIC_MODULE_SHARED_ADDRESS=WF_MUSIC_MODULE_SHARED&0x1fffffffu,
 WF_MUSIC_MODULE_MAGIC=0x57464d53u,
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
 * Four loads: selected video-RAM program, private BSS, shared page, HWRAM
 * code/state. Both entry functions stay inside the executable segment.
 * The header carries exact profile bounds; the loader rejects another
 * memory profile even when its ABI number is also4. Compile both together. */
#endif
