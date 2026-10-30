#ifndef WF_CORE_PHASES_H
#define WF_CORE_PHASES_H
#include "wf_runtime.h"
wf_status_t wf_phase_damage(wf_runtime_t*,uint32_t,void*);
wf_status_t wf_phase_vblank_stub(wf_runtime_t*,uint32_t,void*);
#endif
