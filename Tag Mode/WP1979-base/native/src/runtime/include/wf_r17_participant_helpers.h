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
/* One serialized game-owner workspace, retained through synchronous output
 * callbacks and rollback. A busy acquire returns BAD_STATE without writing
 * the output pointer or reinitializing either record. Only the successful
 * owner may release it; TAG and Royal may not nest in either direction. */
#define WF_PARTICIPANT_LEASE_API 1
enum { WF_PARTICIPANT_TAG=0x6e34u, WF_PARTICIPANT_ROYAL=0x9132u };
typedef struct wf_participant_workspace {
 wf_participant_machine_t machine;
 wf_runtime_t snapshot;
} wf_participant_workspace_t;
wf_status_t wf_participant_workspace_acquire(unsigned,wf_participant_workspace_t **);
wf_status_t wf_participant_workspace_release(unsigned,wf_participant_workspace_t *);
void wf_participant_init(wf_participant_machine_t *,wf_runtime_t *,wf_participant_read16_fn,void *);
uint32_t wf_participant_read(wf_participant_machine_t *,uint32_t,unsigned);
void wf_participant_write(wf_participant_machine_t *,uint32_t,uint32_t,unsigned);
void wf_participant_1079e(wf_participant_machine_t *);
void wf_participant_7720(wf_participant_machine_t *);
void wf_participant_782c(wf_participant_machine_t *);
void wf_participant_75ee(wf_participant_machine_t *);
void wf_participant_7506(wf_participant_machine_t *);
#endif
