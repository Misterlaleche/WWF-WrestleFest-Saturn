/* Sega ST-162 command windows. FRT is observed, never reconfigured.
 * The single history is deliberately external to the transport translation
 * unit: CDFS and any probe/private transport copy share the same budget.
 */
#include "wp2015_cd_rate.h"
#include <stddef.h>

#ifdef WP2015_CD_RATE_TEST
extern uint16_t wp2015_cd_rate_test_frt(void);
extern uint8_t wp2015_cd_rate_test_divider(void);
extern void wp2015_cd_rate_test_idle(void);
#define read_frt wp2015_cd_rate_test_frt
#define read_divider wp2015_cd_rate_test_divider
#define rate_idle wp2015_cd_rate_test_idle
#else
#include <cpu/frt.h>
static uint16_t read_frt(void){
    volatile cpu_ioregs_t *const r=(volatile cpu_ioregs_t *)CPU_IOREG_BASE;
    uint16_t high,low,check;
    do{high=r->frch;low=r->frcl;check=r->frch;}while(high!=check);
    return (uint16_t)((high<<8)|low);
}
static uint8_t read_divider(void){
    return ((volatile cpu_ioregs_t *)CPU_IOREG_BASE)->tcr&3u;
}
static void rate_idle(void){__asm__ volatile("nop");}
#endif

typedef struct {
    uint16_t stamp[60];
    uint32_t last_vblank;
    uint8_t head,count,divider;
} rate_history_t;
static rate_history_t history;
static volatile uint32_t *frame_clock;
_Static_assert(sizeof(rate_history_t)==128,"CD history storage budget");

/* ceil(28,636,364 Hz * window / divider) +1. The additional tick accounts
 * for the fractional tick lost by sampling an integer FRT counter. Using the
 * maximum Saturn CPU clock also covers the slower320/PAL clock modes.
 * Rows are the native TCR encodings: /8, /32 and /128. */
static const uint16_t windows[3][2]={{23984,59780},{5997,14946},{1500,3738}};
#define RATE_POLL_LIMIT 0x240000u
#define RATE_BAD_CLOCK UINT16_MAX

static void clear_history(void){history.head=0;history.count=0;}
static void age_history(uint32_t vblank){
    uint32_t elapsed=vblank-history.last_vblank;
    /* At least two complete fields elapsed even if the last command was just
     * before a VBlank. A counter rewind is not mistaken for elapsed time. */
    if(history.count && elapsed>=3u && elapsed<0x80000000u)clear_history();
}
static uint16_t required_delay(uint16_t now,uint32_t vblank,uint8_t divider,unsigned drain){
    age_history(vblank);
    if(divider>2u || (history.count && divider!=history.divider))return RATE_BAD_CLOCK;
    if(!history.count)return 0;
    if(drain){
        unsigned last=history.head?history.head-1u:59u;
        uint16_t elapsed=(uint16_t)(now-history.stamp[last]);
        return elapsed<windows[divider][1]?(uint16_t)(windows[divider][1]-elapsed):0;
    }
    uint16_t wait=0;
    if(history.count>=30u){
        unsigned at=history.head>=30u?history.head-30u:history.head+30u;
        uint16_t elapsed=(uint16_t)(now-history.stamp[at]);
        if(elapsed<windows[divider][0])wait=(uint16_t)(windows[divider][0]-elapsed);
    }
    if(history.count==60u){
        uint16_t elapsed=(uint16_t)(now-history.stamp[history.head]);
        if(elapsed<windows[divider][1]){
            uint16_t long_wait=(uint16_t)(windows[divider][1]-elapsed);
            if(long_wait>wait)wait=long_wait;
        }
    }
    return wait;
}
static int wait_history(unsigned drain){
    if(!frame_clock)return WP2015_CD_RATE_NOT_BOUND;
    for(uint32_t polls=0;polls<RATE_POLL_LIMIT;polls++){
        uint8_t divider=read_divider();
        uint16_t delay=required_delay(read_frt(),*frame_clock,divider,drain);
        if(delay==RATE_BAD_CLOCK)return WP2015_CD_RATE_CLOCK;
        if(!delay)return 0;
        rate_idle();
    }
    return WP2015_CD_RATE_TIMER_TIMEOUT;
}
#ifdef WP2015_CD_RATE_TRACE
/* Native probe only. Release builds contain neither callback nor trace data. */
extern void wp2015_cd_rate_trace(uint16_t,uint8_t,uint32_t);
#endif
void wp2015_cd_rate_issued(void){
    /* Caller has just written CR4 with IRQs masked. No CD owner or FRT reset
     * may intervene between successful admission and this notification. */
    uint16_t now=read_frt();
    history.stamp[history.head]=now;
    if(++history.head==60u)history.head=0;
    if(history.count<60u)++history.count;
    history.divider=read_divider();history.last_vblank=*frame_clock;
#ifdef WP2015_CD_RATE_TRACE
    wp2015_cd_rate_trace(now,history.divider,history.last_vblank);
#endif
}
int wp2015_cd_rate_wait(void){return wait_history(0);}
int wp2015_cd_rate_before_frt_reset(void){
    int rc=wait_history(1);
    if(!rc)clear_history();
    return rc;
}
int wp2015_cd_rate_bind(volatile uint32_t *video_clock){
    if(!video_clock)return WP2015_CD_RATE_NOT_BOUND;
    if(frame_clock){int rc=wp2015_cd_rate_before_frt_reset();if(rc)return rc;}
    if(read_divider()>2u)return WP2015_CD_RATE_CLOCK;
    frame_clock=video_clock;
    clear_history();
    /* A fresh/repeated bind may follow untracked SDK initialization commands.
     * Seed a conservative barrier at the current time, then drain it. */
    wp2015_cd_rate_issued();
    return wp2015_cd_rate_before_frt_reset();
}
