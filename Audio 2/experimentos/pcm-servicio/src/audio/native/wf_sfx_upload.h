#ifndef WF_SFX_UPLOAD_H
#define WF_SFX_UPLOAD_H
#include "wf_native_host.h"
/* Boot/normal context, before queue initialization. read returns exactly bytes
 * or false; it may perform CD I/O while the 68000 is running and SFX unbound.
 * The owner is held during copies; callback must not reenter audio/SMPC APIs. */
typedef bool (*wf_sfx_read_fn)(uint32_t offset,void *dst,size_t bytes,void *user);
bool wf_sfx_upload(wf_native_host_t *,uint32_t bytes,wf_sfx_read_fn,void *user);
/* Caller-owned aligned scratch lets the CD reader transfer full 16-sector
 * blocks directly while the sound CPU is stopped and the scene lease is free. */
bool wf_sfx_upload_bulk(wf_native_host_t *,uint32_t bytes,wf_sfx_read_fn,
                        void *user,uint8_t *scratch,size_t capacity);
#endif
