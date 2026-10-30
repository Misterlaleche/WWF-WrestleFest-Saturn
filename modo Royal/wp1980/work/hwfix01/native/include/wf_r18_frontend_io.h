#ifndef WF_R18_FRONTEND_IO_H
#define WF_R18_FRONTEND_IO_H
#include "wf_r18_frontend_boot.h"
#include "wf_r18_bootstrap.h"
#include "wf_r18_video.h"

typedef struct wf_r18_frontend_io wf_r18_frontend_io_t;
typedef wf_status_t (*wf_r18_frontend_platform_fn)(wf_r18_frontend_io_t *,uint32_t);
/* Raw frontend RAM remains authoritative. The runtime is a borrowed view
 * for the existing video/credit owners; no gameplay reset is performed.
 * Remaining world/render/sync owners must be supplied by platform_call. */
struct wf_r18_frontend_io {
 uint8_t *ram;
 wf_r17_m68k_shadow_t *regs;
 uint32_t *a;
 wf_runtime_t *runtime;
 wf_r18_video_t *video;
 wf_r18_bootstrap_context_t *bootstrap;
 wf_r18_frontend_platform_fn platform_call;
 void *user;
 uint32_t blocked_dependency;
 uint8_t background_cleared,sprites_cleared;
};
wf_status_t wf_r18_frontend_io_call(wf_r18_frontend_io_t *,uint32_t owner);
wf_status_t wf_r18_frontend_io_read_ram(wf_runtime_t *,uint32_t,uint16_t *,void *);
#endif
