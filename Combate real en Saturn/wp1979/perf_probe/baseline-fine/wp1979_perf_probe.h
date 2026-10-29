#ifndef WP1979_PERF_PROBE_H
#define WP1979_PERF_PROBE_H
#include <stdint.h>
#include <yaul.h>
enum {
 P_RUNTIME,P_PRESENT,P_WORLD,P_RING,P_PALETTE,P_WAIT1,P_WAIT2,P_CD_SPRITE,P_CD_OTHER,
 P_META,P_PLAN,P_RETAIN,P_UPLOADS,P_PAL_UPLOAD,P_COMMANDS,P_BACKEND_END,P_DECODE,
 P_CHANGED_UPLOAD,P_TEXTURE_DMA,P_CMD_PUT,P_CMD_WAIT,P_RENDER_REQUEST,P_SYNC1,P_SYNC2,
 P_PRE_RUNTIME,P_DISPLAY_IRQ,P_HUD_IRQ,P_HUD_COMMIT,P_ELAPSED,P_CROP,P_COMPARE,P_RESERVED,
 P_STAGES=32
};
enum { C_TILES,C_COMMANDS,C_RECORDS,C_DECODES,C_CHANGED,C_DMA_BYTES,C_DMAS,C_NARROW };
typedef struct {
 uint32_t frame,start_clock,end_clock,pixel_area;
 uint16_t ticks[P_STAGES],counts[8],stamp[12];
} wp1979_profile_row_t;
extern volatile unsigned wf_perf_active;
extern volatile wp1979_profile_row_t profile_current;
extern volatile uint16_t profile_last_out,profile_phase;
extern volatile uint32_t profile_totals[P_STAGES],profile_calls[P_STAGES];
void wf_perf_mark(unsigned,uint16_t,uint32_t);
void wf_perf_begin(uint32_t,uint32_t);
void wf_perf_finish(uint32_t,uint32_t);
void wf_perf_vblank_out(void);
static inline uint16_t wf_perf_start(void){return wf_perf_active?cpu_frt_count_get():0u;}
static inline void wf_perf_count(unsigned i,unsigned n){if(wf_perf_active)profile_current.counts[i]=(uint16_t)(profile_current.counts[i]+n);}
static inline void wf_perf_stamp(unsigned i){if(wf_perf_active)profile_current.stamp[i]=(uint16_t)(cpu_frt_count_get()-profile_last_out);}
#endif
