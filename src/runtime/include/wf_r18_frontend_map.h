#ifndef WF_R18_FRONTEND_MAP_H
#define WF_R18_FRONTEND_MAP_H
#include "wf_r18_video.h"
typedef wf_status_t (*wf_r18_frontend_store_fn)(void *,uint32_t,uint32_t,unsigned);
typedef struct wf_r18_frontend_map {
 uint8_t *ram;wf_r17_m68k_shadow_t *regs;uint32_t *a;
 wf_r18_rom_read_fn read_rom;wf_r18_frontend_store_fn store;void *user;
 uint32_t blocked_dependency;
} wf_r18_frontend_map_t;
/* Literal world-map owners $26E66, $26A42, $26DAC and $26E1A.
 * RAM16KB is authoritative, including source/destination queues. Stores
 * include original board scroll/priority words and exact BG/FG cell bytes.
 * The platform owns VBlank and uploads; no frames or RNG are synthesized. */
wf_status_t wf_r18_frontend_map_call(wf_r18_frontend_map_t *,uint32_t owner);
#endif
