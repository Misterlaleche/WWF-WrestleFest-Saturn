#ifndef WF_NATIVE_QUEUE_H
#define WF_NATIVE_QUEUE_H
#include "wf_native_host.h"
/* One main-context producer/consumer. VBlank never touches this queue.
 * This queue is the host mailbox's only command writer; do not mix it with
 * synchronous host_command/WP24 control calls on the same mailbox. */
typedef struct {
    wf_native_host_t *host;
    uint16_t words[64],sequence,last_unimplemented;
    unsigned head,tail,used;
    uint32_t submitted,completed,unimplemented,priority_rejected,overflows,invalid,sent_at;
    bool pending,fault;
} wf_native_queue_t;
bool wf_native_queue_init(wf_native_queue_t *,wf_native_host_t *);
bool wf_native_queue_push(wf_native_queue_t *,uint16_t raw);
/* Bounded polling; never sleeps, waits for ack, performs CD I/O or calls SMPC.
 * Unimplemented arcade commands are acknowledged and counted, never rendered
 * using the diagnostic FM instrument. Timeout poisons the host until reboot. */
bool wf_native_queue_service(wf_native_queue_t *,uint32_t now_us);
#endif
