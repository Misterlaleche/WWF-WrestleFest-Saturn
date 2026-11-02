#include "wf_music_seq.h"
#include <assert.h>
#include <inttypes.h>
#include <stdio.h>
extern unsigned wf_audit_batch_core(z80 *,unsigned,uint32_t);
extern void wf_audit_cold_step(z80 *);
uint64_t wf_audit_since_service,wf_audit_retired,wf_audit_services,wf_audit_max_service;
unsigned wf_audit_max_grant;
int wf_audit_service_enabled=1;

unsigned wf_z80_control_batch(z80 *z,unsigned budget,uint32_t limit)
{
 if(wf_audit_service_enabled){assert(budget<=512);if(budget>wf_audit_max_grant)wf_audit_max_grant=budget;}
 unsigned n=wf_audit_batch_core(z,budget,limit);assert(n<=budget);
 wf_audit_since_service+=n;wf_audit_retired+=n;return n;
}
void z80_step(z80 *z)
{
 wf_audit_cold_step(z);++wf_audit_since_service;++wf_audit_retired;
}
void wf_service_audit(void)
{
 if(wf_audit_since_service>wf_audit_max_service)wf_audit_max_service=wf_audit_since_service;
 assert(wf_audit_since_service<=512);wf_audit_since_service=0;++wf_audit_services;
}
void wf_service_audit_report(void)
{
 printf("SERVICE_AUDIT original_instructions=%"PRIu64" callbacks=%"PRIu64" max_between_callbacks=%"PRIu64" max_grant=%u\n",
        wf_audit_retired,wf_audit_services,wf_audit_max_service,wf_audit_max_grant);
}
