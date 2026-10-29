/* Fine FRT diagnostic only. Original v3 gameplay and every wait are retained. */
#include <yaul.h>
#include <string.h>
#include "wp1979_perf_probe.h"
#include "wf_gameplay_bindings.h"
#include "wf_r18_bootstrap.h"
#include "wf_r18_world.h"
#include "wf_r18_present.h"
#include "wf_r18_cdfs.h"
#include "wf_r18_ring.h"
#include "wf_r18_hud.h"
#include "wf_r18_display.h"
#ifndef WF_R18_PROFILE_START
#define WF_R18_PROFILE_START 7400u
#endif
#ifndef WF_R18_PROFILE_STOP
#define WF_R18_PROFILE_STOP 7800u
#endif
volatile uint32_t profile_totals[P_STAGES],profile_calls[P_STAGES];
volatile uint32_t profile_frames,profile_vblanks,profile_start_vblank,profile_done;
volatile uint32_t profile_vblank_ticks,profile_vblank_count,profile_marks;
volatile unsigned wf_perf_active;
volatile uint16_t profile_last_out,profile_phase;
volatile wp1979_profile_row_t profile_current;
volatile wp1979_profile_row_t profile_rows[WF_R18_PROFILE_STOP-WF_R18_PROFILE_START];
static uint16_t frame_started;
static wf_status_t profile_frame(wf_runtime_t *);
static wf_status_t profile_present(wf_runtime_t *,wf_actor_raw_t *const *,size_t,void *);
static wf_status_t profile_world(wf_runtime_t *,wf_r18_world_ctx_t *);
static int profile_ring(int16_t,int16_t,int16_t,int16_t,uint8_t);
static wf_status_t profile_palette(wf_r18_video_t *,unsigned);
static int profile_file(const char *,uint32_t,void *,size_t);
static wf_status_t profile_boot(wf_runtime_t *,wf_r18_bootstrap_context_t *,const wf_r18_launch_config_t *);
static wf_status_t profile_display(wf_runtime_t *,wf_r18_display_t *);
static wf_status_t profile_hud_irq(wf_runtime_t *,wf_r18_video_t *);
static wf_status_t profile_hud_commit(wf_r18_video_t *);
static void profile_vdp1wait(void),profile_vdp2wait(void),profile_sync1(void),profile_sync2(void);
#define wf_runtime_frame profile_frame
#define wf_r18_present_batch profile_present
#define wf_r18_world_commit profile_world
#define wf_r18_ring_update profile_ring
#define wf_r18_palettes_commit profile_palette
#define wf_r18_file_read profile_file
#define wf_r18_direct_boot profile_boot
#define wf_r18_display_irq profile_display
#define wf_r18_hud_irq profile_hud_irq
#define wf_r18_hud_commit profile_hud_commit
#define vdp1_sync_wait profile_vdp1wait
#define vdp2_sync_wait profile_vdp2wait
#define vdp1_sync profile_sync1
#define vdp2_sync profile_sync2
#include "../../src/wp1979_entry.c"
#undef wf_runtime_frame
#undef wf_r18_present_batch
#undef wf_r18_world_commit
#undef wf_r18_ring_update
#undef wf_r18_palettes_commit
#undef wf_r18_file_read
#undef wf_r18_direct_boot
#undef wf_r18_display_irq
#undef wf_r18_hud_irq
#undef wf_r18_hud_commit
#undef vdp1_sync_wait
#undef vdp2_sync_wait
#undef vdp1_sync
#undef vdp2_sync
void wf_perf_mark(unsigned i,uint16_t start,uint32_t units){
 if(wf_perf_active){uint16_t elapsed=(uint16_t)(cpu_frt_count_get()-start);profile_totals[i]+=elapsed;profile_calls[i]+=units;profile_current.ticks[i]=(uint16_t)(profile_current.ticks[i]+elapsed);++profile_marks;}
}
void wf_perf_begin(uint32_t frame,uint32_t clock){
 if(wf_perf_active||frame<WF_R18_PROFILE_START||frame>=WF_R18_PROFILE_STOP)return;
 memset((void*)&profile_current,0,sizeof profile_current);
 profile_current.frame=frame;profile_current.start_clock=clock;
 if(!profile_frames)profile_start_vblank=clock;
 frame_started=cpu_frt_count_get();profile_phase=0;wf_perf_active=1;wf_perf_stamp(0);
}
void wf_perf_finish(uint32_t frame,uint32_t clock){
 if(!wf_perf_active||frame<=profile_current.frame)return;
 wf_perf_mark(P_ELAPSED,frame_started,1);profile_current.end_clock=clock;
 wf_perf_active=0;profile_rows[profile_frames]=profile_current;++profile_frames;
 profile_vblanks=clock-profile_start_vblank;
 if(frame>=WF_R18_PROFILE_STOP){profile_done=1;for(;;)smpc_peripheral_process();}
}
void wf_perf_vblank_out(void){
 static const uint16_t masks[8]={1,2,4,8,16,32,64,128};uint16_t now=cpu_frt_count_get();
 if(wf_perf_active){profile_vblank_ticks+=(uint16_t)(now-profile_last_out);++profile_vblank_count;profile_current.stamp[11]|=masks[profile_phase&7u];}
 profile_last_out=now;
}
static void probe_vblank_in(void *unused){(void)unused;static const uint16_t masks[8]={1,2,4,8,16,32,64,128};if(wf_perf_active)profile_current.stamp[10]|=masks[profile_phase&7u];}
static wf_status_t profile_boot(wf_runtime_t*r,wf_r18_bootstrap_context_t*c,const wf_r18_launch_config_t*l){wf_status_t s=wf_r18_direct_boot(r,c,l);cpu_frt_init(CPU_FRT_CLOCK_DIV_128);profile_last_out=cpu_frt_count_get();vdp_sync_vblank_in_set(probe_vblank_in,NULL);return s;}
static wf_status_t profile_frame(wf_runtime_t*r){uint16_t t=wf_perf_start();wf_status_t s=wf_runtime_frame(r);wf_perf_mark(P_RUNTIME,t,1);return s;}
static wf_status_t profile_present(wf_runtime_t*r,wf_actor_raw_t *const*a,size_t n,void*u){uint16_t t=wf_perf_start();wf_status_t s=wf_r18_present_batch(r,a,n,u);wf_perf_mark(P_PRESENT,t,1);return s;}
static wf_status_t profile_world(wf_runtime_t*r,wf_r18_world_ctx_t*c){uint16_t t=wf_perf_start();wf_status_t s=wf_r18_world_commit(r,c);wf_perf_mark(P_WORLD,t,1);return s;}
static int profile_ring(int16_t a,int16_t b,int16_t c,int16_t d,uint8_t p){uint16_t t=wf_perf_start();int s=wf_r18_ring_update(a,b,c,d,p);wf_perf_mark(P_RING,t,1);return s;}
static wf_status_t profile_palette(wf_r18_video_t*v,unsigned role){uint16_t t=wf_perf_start();wf_status_t s=wf_r18_palettes_commit(v,role);wf_perf_mark(P_PALETTE,t,1);return s;}
static int profile_file(const char*n,uint32_t off,void*p,size_t bytes){uint16_t t=wf_perf_start();int s=wf_r18_file_read(n,off,p,bytes);wf_perf_mark(!strcmp(n,"SPRITES.PAK")?P_CD_SPRITE:P_CD_OTHER,t,1);return s;}
static wf_status_t profile_display(wf_runtime_t*r,wf_r18_display_t*d){uint16_t t=wf_perf_start();wf_status_t s=wf_r18_display_irq(r,d);wf_perf_mark(P_DISPLAY_IRQ,t,1);return s;}
static wf_status_t profile_hud_irq(wf_runtime_t*r,wf_r18_video_t*v){uint16_t t=wf_perf_start();wf_status_t s=wf_r18_hud_irq(r,v);wf_perf_mark(P_HUD_IRQ,t,1);return s;}
static wf_status_t profile_hud_commit(wf_r18_video_t*v){uint16_t t=wf_perf_start();wf_status_t s=wf_r18_hud_commit(v);wf_perf_mark(P_HUD_COMMIT,t,1);return s;}
static void profile_sync1(void){if(wf_perf_active){profile_phase=2;wf_perf_stamp(2);profile_current.stamp[9]=*(volatile uint16_t*)(VDP2_IOREG_BASE+4u);}uint16_t t=wf_perf_start();vdp1_sync();wf_perf_mark(P_SYNC1,t,1);}
static void profile_sync2(void){uint16_t t=wf_perf_start();vdp2_sync();wf_perf_mark(P_SYNC2,t,1);if(wf_perf_active)profile_phase=3;}
static void profile_vdp1wait(void){if(wf_perf_active){profile_phase=4;wf_perf_stamp(3);profile_current.stamp[8]=vdp1_transfer_status_get().raw;}uint16_t t=wf_perf_start();vdp1_sync_wait();wf_perf_mark(P_WAIT1,t,1);if(wf_perf_active){profile_phase=5;wf_perf_stamp(4);}}
static void profile_vdp2wait(void){if(wf_perf_active){profile_phase=6;wf_perf_stamp(5);}uint16_t t=wf_perf_start();vdp2_sync_wait();wf_perf_mark(P_WAIT2,t,1);if(wf_perf_active){profile_phase=7;wf_perf_stamp(6);}}
