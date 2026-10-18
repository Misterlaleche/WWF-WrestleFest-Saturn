#ifndef WF_R17_PARTICIPANT_HELPERS_H
#define WF_R17_PARTICIPANT_HELPERS_H
#include "wf_r17_post7a00.h"
/* Native 68000-width register contract shared by participant owners. */
#define WF_PARTICIPANT_WRITES 4096u
typedef wf_status_t (*wf_participant_read16_fn)(wf_runtime_t *,uint32_t,uint16_t *,void *);
typedef struct wf_participant_machine {
 wf_runtime_t *rt;
 uint32_t d[8],a[7];
 uint8_t z,n,c;
 wf_status_t status;
 uint32_t failed_address;
 wf_participant_read16_fn read16;
 void *user;
 wf_r17_fg0_write_t writes[WF_PARTICIPANT_WRITES];
 size_t write_count;
} wf_participant_machine_t;
void wf_participant_init(wf_participant_machine_t *,wf_runtime_t *,wf_participant_read16_fn,void *);
uint32_t wf_participant_read(wf_participant_machine_t *,uint32_t,unsigned);
void wf_participant_write(wf_participant_machine_t *,uint32_t,uint32_t,unsigned);
void wf_participant_1079e(wf_participant_machine_t *);
void wf_participant_7720(wf_participant_machine_t *);
void wf_participant_782c(wf_participant_machine_t *);
void wf_participant_75ee(wf_participant_machine_t *);
void wf_participant_7506(wf_participant_machine_t *);
#endif
