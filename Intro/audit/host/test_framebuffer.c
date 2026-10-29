/* Real controller, fake memory-mapped hardware only. A missed deadline must
 * blank before flipping, and command DMA must finish before PTMR=1. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <yaul.h>
#include "wp0_framebuffer.h"
volatile vdp1_ioregs_t test_vdp1;
volatile vdp2_ioregs_t test_vdp2;
static void (*sprite_end)(void);
static uint16_t timer;
static uint32_t interrupt_mask;
static unsigned dma_pending,dma_waited,blanked,deadline_during_dma;
static uint32_t commands[24];

void scu_ic_ihr_set(unsigned vector,void (*handler)(void)) {
    assert(vector==0x4D); sprite_end=handler;
}
void scu_ic_mask_chg(uint32_t a,uint32_t b) { (void)a; (void)b; }
void scu_ic_status_set(scu_ic_status_reg_t value) { (void)value; }
uint32_t cpu_intc_mask_get(void) { return interrupt_mask; }
void cpu_intc_mask_set(uint32_t value) { interrupt_mask=value; }
uint16_t cpu_frt_count_get(void) { return timer; }
void cpu_cache_purge(void) { }
void scu_dma_transfer(unsigned level,void *dst,const void *src,size_t bytes) {
    assert(level==0 && (uintptr_t)dst==0x25C00000U);
    assert(src==commands && bytes==96);
    dma_pending=1;
}
void scu_dma_transfer_wait(unsigned level) {
    assert(level==0 && dma_pending && test_vdp1.ptmr!=1);
    dma_pending=0; dma_waited++;
    if (deadline_during_dma) wp0_framebuffer_vblank_in();
}
void vdp2_tvmd_display_clear(void) { blanked++; }
static void reset(void) {
    memset((void *)&test_vdp1,0,sizeof(test_vdp1));
    test_vdp2.tvmd=0x8010;
    sprite_end=NULL; timer=0; interrupt_mask=0;
    dma_pending=dma_waited=blanked=deadline_during_dma=0;
    wp0_framebuffer_initialize();
}
static void test_preparation_deadline(void) {
    reset();
    assert(wp0_framebuffer_begin());
    wp0_framebuffer_vblank_in();
    assert(wp0_framebuffer_status.error==WP0_FB_PREPARE_LATE);
    assert(blanked==1 && !(test_vdp2.tvmd&0x8000));
    assert(!wp0_framebuffer_submit(commands,sizeof(commands)));
}
static void test_draw_deadline(void) {
    reset(); assert(wp0_framebuffer_begin());
    assert(wp0_framebuffer_submit(commands,sizeof(commands)));
    assert(dma_waited==1 && test_vdp1.ptmr==1);
    test_vdp1.edsr=0; timer=7000;
    wp0_framebuffer_vblank_in();
    assert(wp0_framebuffer_status.error==WP0_FB_DRAW_LATE);
    assert(blanked==1 && !(test_vdp2.tvmd&0x8000));
    assert(wp0_framebuffer_status.last_draw_ticks==7000);
}
static void test_completed_render_and_wrap(void) {
    reset(); assert(wp0_framebuffer_begin()); timer=65530;
    assert(wp0_framebuffer_submit(commands,sizeof(commands)));
    assert(sprite_end); timer=14; test_vdp1.edsr=2; sprite_end();
    assert(wp0_framebuffer_status.last_draw_ticks==20);
    assert(wp0_framebuffer_status.max_draw_ticks==20);
    assert(wp0_framebuffer_status.draw_irq_count==1);
    wp0_framebuffer_vblank_in();
    assert(!wp0_framebuffer_status.error && !blanked);
    assert(wp0_framebuffer_begin());
}
static void test_cef_fallback_without_irq(void) {
    reset(); assert(wp0_framebuffer_begin());
    assert(wp0_framebuffer_submit(commands,sizeof(commands)));
    timer=1234; test_vdp1.edsr=2;
    wp0_framebuffer_vblank_in();
    assert(!wp0_framebuffer_status.error && !blanked);
    assert(wp0_framebuffer_status.cef_fallback_count==1);
    assert(wp0_framebuffer_status.last_draw_ticks==1234);
}
static void test_dma_overrun_never_launches(void) {
    reset(); assert(wp0_framebuffer_begin()); deadline_during_dma=1;
    assert(!wp0_framebuffer_submit(commands,sizeof(commands)));
    assert(wp0_framebuffer_status.error==WP0_FB_PREPARE_LATE);
    assert(test_vdp1.ptmr==0);
}
static void test_pause_and_duplicate_submission(void) {
    reset(); wp0_framebuffer_vblank_in(); assert(!wp0_framebuffer_status.error);
    assert(wp0_framebuffer_begin());
    assert(wp0_framebuffer_submit(commands,sizeof(commands)));
    assert(!wp0_framebuffer_submit(commands,sizeof(commands)));
    assert(wp0_framebuffer_status.error==WP0_FB_BAD_SUBMISSION);
    reset(); assert(wp0_framebuffer_begin()); wp0_framebuffer_stop();
    wp0_framebuffer_vblank_in(); assert(!wp0_framebuffer_status.error);
}
int main(void) {
    test_preparation_deadline(); test_draw_deadline();
    test_completed_render_and_wrap(); test_cef_fallback_without_irq();
    test_dma_overrun_never_launches(); test_pause_and_duplicate_submission();
    puts("framebuffer controller: 6 behavior tests passed");
}
