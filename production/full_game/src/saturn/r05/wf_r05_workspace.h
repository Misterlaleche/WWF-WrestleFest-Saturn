#ifndef WF_R05_WORKSPACE_H
#define WF_R05_WORKSPACE_H
#include <stdint.h>
/* Exclusive payload, not a second allocation. Every member DMA-aligned. */
typedef struct {
 uint32_t maps[3][4096] __attribute__((aligned(32)));
 uint32_t palette[1024] __attribute__((aligned(32)));
 uint32_t lines[2][240][2] __attribute__((aligned(32)));
 uint16_t sprites[1021*4] __attribute__((aligned(32)));
 uint8_t packet[131072] __attribute__((aligned(32)));
 uint8_t loop_cache[262144] __attribute__((aligned(32)));
 uint8_t commands[1024*32] __attribute__((aligned(32)));
} wf_r05_workspace_t;
_Static_assert(sizeof(wf_r05_workspace_t)==491264u,"R05 aligned payload budget");
#endif
