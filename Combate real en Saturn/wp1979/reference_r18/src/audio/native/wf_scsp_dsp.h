#ifndef WF_SCSP_DSP_H
#define WF_SCSP_DSP_H
#include "wf_scsp_sfx.h"
enum {WF_DSP_CLOCK=0xb010,WF_MUSIC_RING_HEADER=0xb020,WF_MUSIC_RING=0xb100,WF_MUSIC_RING_COUNT=256};
bool wf_scsp_dsp_load(wf_sfx_write16_fn,void *);
bool wf_scsp_dsp_voice(unsigned,bool,unsigned,void *);
void wf_scsp_dsp_clock_run(void);
#endif
