#ifndef WF_PACER_H
#define WF_PACER_H
#include "wf_arcade_clock.h"
typedef struct {
 wf_arcade_clock_t clock;
 uint32_t last_field,load_depth;
 uint64_t load_fields;
} wf_pacer_t;
bool wf_pacer_init(wf_pacer_t *,uint32_t physical_field,uint32_t phase);
bool wf_pacer_account(wf_pacer_t *,uint32_t physical_field);
bool wf_pacer_take(wf_pacer_t *);
bool wf_pacer_load_begin(wf_pacer_t *,uint32_t physical_field);
bool wf_pacer_load_end(wf_pacer_t *,uint32_t physical_field);
#endif
