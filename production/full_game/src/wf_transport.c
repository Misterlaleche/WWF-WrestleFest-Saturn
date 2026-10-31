#include "wf_transport.h"
#include <stdbool.h>
extern wf_status_t wf_frontend_call_reference(wf_r18_frontend_driver_t *,uint32_t);
static wf_transport_hook_t hook;
void wf_transport_bind(wf_transport_hook_t h){hook=h;}
wf_status_t wf_r18_frontend_driver_call(wf_r18_frontend_driver_t*d,uint32_t owner){
 /* Only synchronous clear/map owners. Never fade, wait, actor or IRQ owners. */
 bool bulk=owner==0x1f6c||owner==0x1f9e||owner==0x1fc0||owner==0x1fde||owner==0x26e66;
 if(!hook||!bulk)return wf_frontend_call_reference(d,owner);
 wf_status_t st=hook(d,owner,1,WF_OK);if(st!=WF_OK)return st;
 st=wf_frontend_call_reference(d,owner);
 wf_status_t end=hook(d,owner,0,st);return st==WF_OK?end:st;
}
