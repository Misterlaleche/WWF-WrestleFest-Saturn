#ifndef WF_MUSIC_ELF_H
#define WF_MUSIC_ELF_H
#include "wf_native_host.h"
#include "wf_music_module.h"
typedef struct {wf_native_segment_t segments[3];uint32_t prepare,run;} wf_music_image_t;
bool wf_music_elf_image(const void *,size_t,wf_music_image_t *);
bool wf_music_elf_copy(const wf_music_image_t *,void (*write16)(uint32_t,uint16_t,void *),void *);
#endif
