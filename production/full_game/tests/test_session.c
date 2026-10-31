#include "wf_session.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static wf_session_t session;
static uint8_t cold[0x4000];
static unsigned writes;
static wf_status_t rom(void*u,uint32_t a,void*p,size_t n){(void)u;(void)a;(void)p;(void)n;return WF_BLOCKED;}
static wf_status_t read_bus(void*u,uint32_t a,uint32_t*p,unsigned n){(void)u;(void)a;(void)p;(void)n;return WF_BLOCKED;}
static wf_status_t store_bus(void*u,uint32_t a,uint32_t v,unsigned n){(void)u;(void)a;(void)v;(void)n;writes++;return WF_BLOCKED;}
static wf_status_t level(void*u,wf_r18_frontend_driver_t*d,unsigned h){(void)u;(void)d;(void)h;return WF_BLOCKED;}
static wf_status_t sync_event(void*u,wf_r18_frontend_driver_t*d){(void)u;(void)d;return WF_BLOCKED;}
static wf_status_t world(void*u,unsigned m){(void)u;(void)m;return WF_BLOCKED;}
static wf_status_t submit(void*u,const wf_r18_sprite_record_t*r,size_t n){(void)u;(void)r;(void)n;return WF_BLOCKED;}
int main(void){
 for(unsigned i=0;i<sizeof cold;i++)cold[i]=(uint8_t)(i*17u+3u);
 wf_session_config_t c={.cold_ram=cold,.cold_ram_bytes=sizeof cold,.ssp=0x1c3bd4,.usp=0x1c3fd4,.sr=0x2700,
  .platform={.read_rom=rom,.read_bus=read_bus,.store_bus=store_bus,.wait_level=level,.wait_vblank=sync_event,.commit=sync_event,.bind_world=world,.submit_records=submit}};
 assert(wf_session_init(NULL,&c)==WF_BAD_STATE);
 assert(wf_session_init(&session,NULL)==WF_BAD_STATE);
 assert(wf_session_step(&session)==WF_BAD_STATE);
 c.cold_ram_bytes--;assert(wf_session_init(&session,&c)==WF_BAD_STATE);c.cold_ram_bytes++;
 c.platform.commit=NULL;assert(wf_session_init(&session,&c)==WF_BAD_STATE);c.platform.commit=sync_event;
 assert(wf_session_init(&session,&c)==WF_OK);
 assert(!memcmp(session.ram,cold,sizeof cold));
 c.cold_ram=session.ram;assert(wf_session_init(&session,&c)==WF_BAD_STATE);
 assert(!memcmp(session.ram,cold,sizeof cold));c.cold_ram=cold;
 assert(session.frontend.ram==session.ram&&session.frontend.control.regs==&session.frontend.regs);
 assert(wf_session_prepare_match(&session)==WF_BAD_STATE);
 assert(wf_session_step(&session)==WF_OK); /* RESET registers; no fabricated seed. */
 assert(!memcmp(session.ram,cold,0x60));
 assert(wf_session_step(&session)==WF_BLOCKED); /* actual bus failure at$69E */
 assert(session.phase==WF_SESSION_FATAL&&session.error_owner==0x69e&&writes==1);
 assert(wf_session_step(&session)==WF_BLOCKED&&writes==1);
 assert(wf_session_prepare_match(&session)==WF_BLOCKED&&writes==1);
 puts("session: explicit cold state, early-match rejection, preserved RAM, terminal dependency failure PASS");
}
