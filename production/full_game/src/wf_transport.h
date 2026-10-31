#ifndef WF_TRANSPORT_H
#define WF_TRANSPORT_H
#include "wf_r18_frontend_driver.h"
typedef wf_status_t (*wf_transport_hook_t)(wf_r18_frontend_driver_t *,uint32_t owner,unsigned begin,wf_status_t result);
/* Optional platform-only load boundary. Original owners/awaits remain intact. */
void wf_transport_bind(wf_transport_hook_t);
#endif
