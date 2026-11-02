#ifndef WF_SCSP_YM_H
#define WF_SCSP_YM_H
#include "wf_scsp_sfx.h"
typedef struct {
 uint8_t reg[256],key[8],amd,pmd;
 uint16_t words[32][12];
 wf_sfx_write16_fn write16;void *user;
 uint32_t writes,range_clamps;uint8_t fault;
} wf_scsp_ym_t;
void wf_scsp_ym_init(wf_scsp_ym_t *,wf_sfx_write16_fn,void *);
bool wf_scsp_ym_write(wf_scsp_ym_t *,uint8_t,uint8_t);
void wf_scsp_ym_stop(wf_scsp_ym_t *);
uint32_t wf_ym_phase_increment(unsigned kc,unsigned kf,unsigned dtmul,unsigned dt2);
uint16_t wf_ym_pitch_word(uint32_t increment);
#endif
