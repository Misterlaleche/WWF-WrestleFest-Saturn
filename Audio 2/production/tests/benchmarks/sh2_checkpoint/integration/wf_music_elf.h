#ifndef WF_MUSIC_ELF_H
#define WF_MUSIC_ELF_H
#include "wf_native_host.h"
#include "wf_music_module.h"
typedef struct {
 wf_native_segment_t segments[4];
 uint32_t prepare,run;
 const void *elf_data;
 size_t elf_bytes;
} wf_music_image_t;
/* Base/capacity come from the master's linker symbols, never from the file.
 * The result borrows the ELF buffer: keep it alive and unchanged until copy
 * returns. Revalidation checks metadata; this API does not seal payload bytes.
 * write16 must finish each store successfully. Validation rejects all loads
 * before the first store; there is no rollback for a failing platform writer. */
bool wf_music_elf_image(const void *,size_t,uint32_t,uint32_t,wf_music_image_t *);
bool wf_music_elf_copy(const wf_music_image_t *,uint32_t,uint32_t,
                       void (*)(uint32_t,uint16_t,void *),void *);
#endif
