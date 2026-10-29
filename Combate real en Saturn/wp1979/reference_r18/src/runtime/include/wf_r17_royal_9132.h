#ifndef WF_R17_ROYAL_9132_H
#define WF_R17_ROYAL_9132_H
#include "wf_r17_participant_helpers.h"
#include "wf_r17_participant_io.h"
/* Serialized with TAG through the participant lease, including callbacks.
 * Busy entry leaves runtime/context untouched. ROM preserves target/index. */
wf_status_t wf_r17_owner_9132(wf_runtime_t *,wf_actor_raw_t *target,uint16_t paid_index,wf_r17_participant_io_t *);
#endif
