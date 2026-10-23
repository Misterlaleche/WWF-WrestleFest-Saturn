#include <stddef.h>
#include <stdint.h>
#include "wf_saturn_yaul_backend.h"
#include "wf_r18_display.h"
const uint32_t graphics_layout[]={
 offsetof(wf_saturn_yaul_backend_t,texture_bytes),
 offsetof(wf_saturn_yaul_backend_t,texture_transfers),
 offsetof(wf_saturn_yaul_backend_t,texture_reused),
 offsetof(wf_r18_display_t,scroll),
 offsetof(wf_runtime_t,g.global_1800),offsetof(wf_runtime_t,g.global_1802),
 offsetof(wf_runtime_t,g.global_17e6),offsetof(wf_runtime_t,g.global_17ee),
 offsetof(wf_runtime_t,g.global_17ea),offsetof(wf_runtime_t,g.global_17f2_word),
 offsetof(wf_r18_video_t,fg0),offsetof(wf_r18_video_t,palettes)
};
