#ifndef WF_R18_PRESENT_H
#define WF_R18_PRESENT_H
#include "wf_r18_video.h"
#include "wf_r18_metasprite.h"
#include "wf_saturn_yaul_backend.h"
typedef int (*wf_r18_file_read_fn)(const char *,uint32_t,void *,size_t);
/* Buffers are borrowed; Saturn caller assigns nonoverlapping LWRAM regions. */
void wf_r18_present_init(wf_r18_file_read_fn,void *rom_cache,size_t rom_bytes,void *tile_cache,size_t tile_bytes);
wf_status_t wf_r18_rom_preload(void);
wf_status_t wf_r18_rom_pages_preload(const uint16_t *pages,size_t count);
wf_status_t wf_r18_rom_extra_pages_preload(const uint16_t *pages,size_t count);
wf_status_t wf_r18_rom_read(void*,uint32_t,void*,size_t);
wf_status_t wf_r18_hot_sprites_load(void);
size_t wf_r18_hot_sprites_bytes(void);
/* Startup only, after successful HOTSPR load and before any tile read or
 * record submission. Borrow one nonzero multiple of128 bytes from the cold
 * lease tail, preserving at least one cold slot. No pointer is published on
 * failure. present_init/hot reload invalidate the secondary pool and restore
 * capacity before changing storage. The secondary consumer owns no cold
 * slot, and must copy each requested128-byte tile synchronously. */
wf_status_t wf_r18_present_secondary_lease(size_t bytes,void **base);
wf_status_t wf_r18_tile_read(uint16_t,const uint8_t **);
wf_status_t wf_r18_present_batch(wf_runtime_t *,wf_actor_raw_t *const *,size_t,void *);
/* Frontend and combat share this allocation-free record workspace. Acquire
 * and fill synchronously between frames, then submit its counted prefix.
 * Capacity512; callers retain scene/queue/register ownership. */
wf_r18_sprite_record_t *wf_r18_present_records_buffer(size_t *capacity);
wf_status_t wf_r18_present_records_commit(wf_r18_video_t *,size_t count);
/* Between frames the VDP1 DMA staging area can be borrowed by the BIOS
 * backup library. The next draw then uploads its tiles afresh. */
void *wf_r18_present_backup_workspace_begin(size_t bytes);
void wf_r18_present_backup_workspace_end(void);
/* Last successfully submitted original record prefix, before chain
 * expansion/clipping. Read before present_init releases the old lease. */
size_t wf_r18_present_records_count(void);
unsigned wf_r18_present_commands(void);
#ifndef WF_PLATFORM_SATURN
/* Host audit: distinct source tiles selected for the last submitted frame. */
size_t wf_r18_present_frame_tiles(uint16_t *out,size_t capacity);
#endif
#endif
