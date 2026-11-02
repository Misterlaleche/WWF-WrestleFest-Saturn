#include "wf_scsp_ym.h"
static wf_scsp_ym_t state;
__declspec(dllexport) void ym_reset(wf_sfx_write16_fn write){wf_scsp_ym_init(&state,write,0);}
__declspec(dllexport) int ym_write(unsigned reg,unsigned value){return wf_scsp_ym_write(&state,(uint8_t)reg,(uint8_t)value);}
