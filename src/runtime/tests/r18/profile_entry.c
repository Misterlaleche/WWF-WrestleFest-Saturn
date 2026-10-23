/* Diagnostic entry only, never linked into the normal/replay delivery.
 * Real gameplay, no skipped phases or synthetic success. Stop after ACK120.
 * FRT /128 wraps after~312ms; each measured hot component is shorter.
 * Totals overlap: runtime includes present, and present includes file reads. */
#include <yaul.h>
#include <string.h>
#include "wf_gameplay_bindings.h"
#include "wf_r18_bootstrap.h"
#include "wf_r18_world.h"
#include "wf_r18_present.h"
#include "wf_r18_cdfs.h"
#include "wf_r18_ring.h"
volatile uint32_t profile_totals[24],profile_calls[24];
volatile uint32_t profile_frames,profile_vblanks,profile_start_vblank;
#ifndef WF_R18_PROFILE_START
#define WF_R18_PROFILE_START 30u
#endif
#ifndef WF_R18_PROFILE_STOP
#define WF_R18_PROFILE_STOP 120u
#endif
static unsigned measure;
static wf_status_t profile_frame(wf_runtime_t *);
static wf_status_t profile_present(wf_runtime_t *,wf_actor_raw_t *const *,size_t,void *);
static wf_status_t profile_world(wf_runtime_t *,wf_r18_world_ctx_t *);
static int profile_ring(int16_t,int16_t,int16_t,int16_t,uint8_t);
static int profile_palette(unsigned,const uint8_t *,size_t);
static int profile_file(const char *,uint32_t,void *,size_t);
static wf_status_t profile_boot(wf_runtime_t *,wf_r18_bootstrap_context_t *,const wf_r18_launch_config_t *);
static void profile_vdp1wait(void),profile_vdp2wait(void);
#define wf_runtime_frame profile_frame
#define wf_r18_present_batch profile_present
#define wf_r18_world_commit profile_world
#define wf_r18_ring_update profile_ring
#define wf_r18_ring_palette_set profile_palette
#define wf_r18_file_read profile_file
#define wf_r18_direct_boot profile_boot
#define vdp1_sync_wait profile_vdp1wait
#define vdp2_sync_wait profile_vdp2wait
#include "../../src/wf_saturn_entry.c"
#undef wf_runtime_frame
#undef wf_r18_present_batch
#undef wf_r18_world_commit
#undef wf_r18_ring_update
#undef wf_r18_ring_palette_set
#undef wf_r18_file_read
#undef wf_r18_direct_boot
#undef vdp1_sync_wait
#undef vdp2_sync_wait
void wf_r18_profile_stage(unsigned i,uint16_t start,uint32_t units){if(measure){profile_totals[i]+=(uint16_t)(cpu_frt_count_get()-start);profile_calls[i]+=units;}}
static void record(unsigned i,uint16_t start){wf_r18_profile_stage(i,start,1);}
static wf_status_t profile_boot(wf_runtime_t*r,wf_r18_bootstrap_context_t*c,const wf_r18_launch_config_t*l){wf_status_t s=wf_r18_direct_boot(r,c,l);cpu_frt_init(CPU_FRT_CLOCK_DIV_128);return s;}
static wf_status_t profile_frame(wf_runtime_t*r){
 if(!measure&&r->g.frame_counter==WF_R18_PROFILE_START){measure=1;profile_start_vblank=video_clock;}
 uint16_t start=cpu_frt_count_get();uint32_t frame=r->g.frame_counter;wf_status_t s=wf_runtime_frame(r);record(0,start);
 if(measure&&r->g.frame_counter>frame){profile_frames++;profile_vblanks=video_clock-profile_start_vblank;}
 if(r->g.frame_counter==WF_R18_PROFILE_STOP){measure=0;for(;;)smpc_peripheral_process();}
 return s;
}
static wf_status_t profile_present(wf_runtime_t*r,wf_actor_raw_t *const*a,size_t n,void*u){uint16_t t=cpu_frt_count_get();wf_status_t s=wf_r18_present_batch(r,a,n,u);record(1,t);return s;}
static wf_status_t profile_world(wf_runtime_t*r,wf_r18_world_ctx_t*c){uint16_t t=cpu_frt_count_get();wf_status_t s=wf_r18_world_commit(r,c);record(2,t);return s;}
static int profile_ring(int16_t a,int16_t b,int16_t c,int16_t d,uint8_t p){uint16_t t=cpu_frt_count_get();int s=wf_r18_ring_update(a,b,c,d,p);record(3,t);return s;}
static int profile_palette(unsigned l,const uint8_t*p,size_t n){uint16_t t=cpu_frt_count_get();int s=wf_r18_ring_palette_set(l,p,n);record(4,t);return s;}
static int profile_file(const char*n,uint32_t off,void*p,size_t bytes){uint16_t t=cpu_frt_count_get();int s=wf_r18_file_read(n,off,p,bytes);record(!strcmp(n,"SPRITES.PAK")?7:8,t);return s;}
static void profile_vdp1wait(void){uint16_t t=cpu_frt_count_get();vdp1_sync_wait();record(5,t);}
static void profile_vdp2wait(void){uint16_t t=cpu_frt_count_get();vdp2_sync_wait();record(6,t);}
