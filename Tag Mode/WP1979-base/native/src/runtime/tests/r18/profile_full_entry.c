/* Diagnostic full-frontend entry only, never linked into the delivery.
 * Real pad, roster, entrances, gameplay and post flow. Stop at the requested
 * frame after recording a bounded window.
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
#include "wf_r18_hud.h"
volatile uint32_t profile_totals[24],profile_calls[24];
volatile uint32_t profile_frames,profile_vblanks,profile_start_vblank;
volatile uint32_t profile_file_event_count,profile_file_events[64][3];
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
static wf_status_t profile_palette(wf_r18_video_t *,unsigned);
static int profile_file(const char *,uint32_t,void *,size_t);
static void profile_vdp1wait(void),profile_vdp2wait(void);
#define wf_runtime_frame profile_frame
#define wf_r18_present_batch profile_present
#define wf_r18_world_commit profile_world
#define wf_r18_ring_update profile_ring
#define wf_r18_palettes_commit profile_palette
#define wf_r18_file_read profile_file
#define vdp1_sync_wait profile_vdp1wait
#define vdp2_sync_wait profile_vdp2wait
#include "../../src/wf_saturn_entry.c"
#undef wf_runtime_frame
#undef wf_r18_present_batch
#undef wf_r18_world_commit
#undef wf_r18_ring_update
#undef wf_r18_palettes_commit
#undef wf_r18_file_read
#undef vdp1_sync_wait
#undef vdp2_sync_wait
void wf_r18_profile_stage(unsigned i,uint16_t start,uint32_t units){if(measure){profile_totals[i]+=(uint16_t)(cpu_frt_count_get()-start);profile_calls[i]+=units;}}
static void record(unsigned i,uint16_t start){wf_r18_profile_stage(i,start,1);}
static wf_status_t profile_frame(wf_runtime_t*r){
 static unsigned frt_initialized;
 if(!frt_initialized){cpu_frt_init(CPU_FRT_CLOCK_DIV_128);frt_initialized=1;}
 if(!measure&&r->g.frame_counter==WF_R18_PROFILE_START){measure=1;profile_start_vblank=video_clock;}
 uint16_t start=cpu_frt_count_get();uint32_t frame=r->g.frame_counter;wf_status_t s=wf_runtime_frame(r);record(0,start);
 if(measure&&r->g.frame_counter>frame){profile_frames++;profile_vblanks=video_clock-profile_start_vblank;}
 if(r->g.frame_counter==WF_R18_PROFILE_STOP){measure=0;for(;;)smpc_peripheral_process();}
 return s;
}
static wf_status_t profile_present(wf_runtime_t*r,wf_actor_raw_t *const*a,size_t n,void*u){uint16_t t=cpu_frt_count_get();wf_status_t s=wf_r18_present_batch(r,a,n,u);record(1,t);return s;}
static wf_status_t profile_world(wf_runtime_t*r,wf_r18_world_ctx_t*c){uint16_t t=cpu_frt_count_get();wf_status_t s=wf_r18_world_commit(r,c);record(2,t);return s;}
static int profile_ring(int16_t a,int16_t b,int16_t c,int16_t d,uint8_t p){uint16_t t=cpu_frt_count_get();int s=wf_r18_ring_update(a,b,c,d,p);record(3,t);return s;}
static wf_status_t profile_palette(wf_r18_video_t *v,unsigned role){uint16_t t=cpu_frt_count_get();wf_status_t s=wf_r18_palettes_commit(v,role);record(4,t);return s;}
static int profile_file(const char*n,uint32_t off,void*p,size_t bytes){
 uint16_t t=cpu_frt_count_get();int sprite=!strcmp(n,"SPRITES.PAK");
 int s=wf_r18_file_read(n,off,p,bytes);record(sprite?7:8,t);
 if(measure&&!sprite&&profile_file_event_count<64u){
  unsigned i=profile_file_event_count++;
  profile_file_events[i][0]=!strcmp(n,"ROM68K.BIN")?1u:!strcmp(n,"ROYSPR.BIN")?2u:3u;
  profile_file_events[i][1]=off;
  profile_file_events[i][2]=(uint32_t)bytes;
 }
 return s;
}
static void profile_vdp1wait(void){uint16_t t=cpu_frt_count_get();vdp1_sync_wait();record(5,t);}
static void profile_vdp2wait(void){uint16_t t=cpu_frt_count_get();vdp2_sync_wait();record(6,t);}
