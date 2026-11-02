#include <stddef.h>
#include "wf_scsp_sfx.h"
const unsigned long sfx_layout[] = {
 sizeof(wf_scsp_sfx_t), offsetof(wf_scsp_sfx_t,underruns),
 offsetof(wf_scsp_sfx_t,frames), offsetof(wf_scsp_sfx_t,ticks),
 offsetof(wf_scsp_sfx_t,bound), offsetof(wf_scsp_sfx_t,fault),
 offsetof(wf_scsp_sfx_t,pcm), sizeof(wf_scsp_sfx_pcm_t),
 offsetof(wf_scsp_sfx_pcm_t,playing), offsetof(wf_scsp_sfx_pcm_t,write_at),
 offsetof(wf_scsp_sfx_pcm_t,played)
};
