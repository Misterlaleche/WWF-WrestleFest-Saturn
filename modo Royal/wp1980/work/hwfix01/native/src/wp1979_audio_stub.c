/* VIDEO-ONLY BASELINE. This object is excluded when WP1979_AUDIO=1. */
#include "wf_r18_scsp.h"
wf_native_queue_t wf_r18_scsp_queue;
bool wf_r18_scsp_boot(volatile uint32_t *v,const void *e,size_t n){(void)v;(void)e;(void)n;return true;}
bool wf_r18_scsp_smpc_owned(void){return false;}
void wf_r18_scsp_vblank(void){}
bool wf_r18_scsp_service(void){return true;}
wf_status_t wf_r18_scsp_audio(wf_runtime_t *r,uint16_t c,void *u){(void)r;(void)c;(void)u;return WF_OK;}
