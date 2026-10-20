#ifndef WF_R18_PRESENT_H
#define WF_R18_PRESENT_H
#include "wf_r18_video.h"
#include "wf_r18_metasprite.h"
#include "wf_saturn_yaul_backend.h"
typedef int (*wf_r18_file_read_fn)(const char *,uint32_t,void *,size_t);
/* Buffers are borrowed; Saturn caller assigns nonoverlapping LWRAM regions. */
void wf_r18_present_init(wf_r18_file_read_fn,void *rom_cache,size_t rom_bytes,void *tile_cache,size_t tile_bytes);
wf_status_t wf_r18_rom_preload(void);
wf_status_t wf_r18_rom_read(void*,uint32_t,void*,size_t);
wf_status_t wf_r18_hot_sprites_load(void);
size_t wf_r18_hot_sprites_bytes(void);
wf_status_t wf_r18_tile_read(uint16_t,const uint8_t **);
wf_status_t wf_r18_present_batch(wf_runtime_t *,wf_actor_raw_t *const *,size_t,void *);
unsigned wf_r18_present_commands(void);
#endif
