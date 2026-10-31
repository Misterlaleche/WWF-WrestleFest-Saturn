#ifndef WF_GROUND_H
#define WF_GROUND_H
#include <stdint.h>
#include "wf_actor.h"
typedef enum { WF_GROUND_STAY=0, WF_GROUND_ADVANCE=1, WF_GROUND_BLOCKED=2 } wf_ground_result_t;
/* Exact direct linked-victim retype for raw $0A/$0B/$10. Low-byte only. */
wf_ground_result_t wf_ground_linked_retype(uint8_t raw_action,wf_actor_raw_t *victim);
/* Proven direct contact37 release paths only; helper-dependent branches fail closed. */
wf_ground_result_t wf_ground_contact_release(uint8_t raw_action,wf_actor_raw_t *actor);
#endif
