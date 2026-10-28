#ifndef WF_MUSIC_MODULE_H
#define WF_MUSIC_MODULE_H
#include <stdint.h>
enum {
 WF_MUSIC_MODULE_BASE=0x05c5c000u,WF_MUSIC_MODULE_END=0x05c80000u,
 WF_MUSIC_MODULE_SHARED=0x25c7f000u,WF_MUSIC_MODULE_MAGIC=0x57464d53u,
 WF_MUSIC_MODULE_ABI=2,WF_MUSIC_BOOTING=1,WF_MUSIC_RUNNING=2,WF_MUSIC_FAULT=3,
 WF_NATIVE_MUSIC_PROFILE=0x4d46u
};
typedef struct {
 volatile uint16_t producer,consumer,commands[64];
 volatile uint16_t state,shutdown;
 volatile uint32_t error,events,steps_hi,steps_lo,skipped_hi,skipped_lo;
 volatile uint16_t generation,last_command;
} wf_music_module_shared_t;
/* Header is32bytes BE: magic, abi16, bytes16, base, end, prepare, run, shared,
 * reserved. Both function addresses must lie in the executable segment. */
#endif
