/* Derived from supplied session14 R05 reference/source; original retained read-only.
 * Silent fixed-attract integration, 2026-11-02. See PROVENANCE.md. */
#include <yaul.h>
#include <string.h>
#include "wp0_framebuffer.h"
#include "wp0_timer.h"

/* Sega VDP1 manual 4.2/4.3/4.4:
 * VBE=0, FBCR=0: swap each field; erase the displayed buffer during scanout.
 * PTMR=1: start only after this field's complete command table is in VRAM.
 * No SDK vdp1_sync calls: AUTO would arm PTMR=2 and start on the old table.
 * Yaul still owns VDP2 register/palette/map commits and its VBlank handlers.
 */
enum { OFF,PREPARING,DRAWING,COMPLETE,COMMITTED,FAILED };
static volatile unsigned phase;
static scu_ic_ihr_t saved_sprite_end;
static uint32_t saved_sprite_mask;
static bool irq_owned;
static volatile uint16_t draw_started;
volatile wp0_framebuffer_status_t wp0_framebuffer_status;

static volatile vdp1_ioregs_t *const regs=(volatile vdp1_ioregs_t *)VDP1_IOREG_BASE;

static void draw_time_record(void)
{
    uint32_t ticks=(uint16_t)(wp0_timer_count_get()-draw_started);
    wp0_framebuffer_status.last_draw_ticks=ticks;
    if (ticks>wp0_framebuffer_status.max_draw_ticks)
        wp0_framebuffer_status.max_draw_ticks=ticks;
}

static void registers_record(void)
{
    wp0_framebuffer_status.deadline_edsr=regs->edsr;
    wp0_framebuffer_status.deadline_modr=regs->modr;
    wp0_framebuffer_status.deadline_copr=regs->copr;
}

static void fail(unsigned error)
{
    if (wp0_framebuffer_status.error) return;
    registers_record();
    wp0_framebuffer_status.error=error;
    phase=FAILED;
    regs->ptmr=0;
    regs->endr=0; /* Any ENDR write terminates plotting. */
    /* Never expose an unfinished backbuffer at the next automatic swap.
     * Update hardware now and the shadow copy that Yaul is about to commit. */
    ((volatile vdp2_ioregs_t *)VDP2_IOREG_BASE)->tvmd&=0x7FFFU;
    vdp2_tvmd_display_clear();
}

static void sprite_end(void)
{
    if (phase!=DRAWING || !(regs->edsr&2U)) return;
    draw_time_record();
    ++wp0_framebuffer_status.draw_irq_count;
    wp0_framebuffer_status.draw_done=1;
    phase=COMPLETE;
}

void wp0_framebuffer_initialize(void)
{
    if(!irq_owned){
        saved_sprite_end=scu_ic_ihr_get(SCU_IC_INTERRUPT_SPRITE_END);
        saved_sprite_mask=scu_ic_mask_get()&SCU_IC_MASK_SPRITE_END;irq_owned=true;
    }
    phase=OFF;
    memset((void *)&wp0_framebuffer_status,0,sizeof(wp0_framebuffer_status));
    regs->ptmr=0;
    regs->endr=0;
    regs->tvmr=0; /* Normal 16-bit, non-rotated, VBE=0. */
    regs->fbcr=0;
    scu_ic_ihr_set(SCU_IC_INTERRUPT_SPRITE_END,sprite_end);
    scu_ic_status_set((scu_ic_status_reg_t)~SCU_IC_IST_SPRITE_END);
    scu_ic_mask_chg(SCU_IC_MASK_ALL&~SCU_IC_MASK_SPRITE_END,0);
}

bool wp0_framebuffer_begin(void)
{
    if (wp0_framebuffer_status.error) return false;
    if (phase!=OFF && phase!=COMMITTED) {
        fail(WP0_FB_BAD_SUBMISSION); return false;
    }
    regs->ptmr=0;
    wp0_framebuffer_status.submitted=0;
    wp0_framebuffer_status.draw_done=0;
    phase=PREPARING;
    return true;
}

bool wp0_framebuffer_submit(const void *commands,unsigned bytes)
{
    if (wp0_framebuffer_status.error) return false;
    if (phase!=PREPARING || !commands || ((uintptr_t)commands&3U) ||
        !bytes || (bytes&31U) || bytes>32768U) {
        fail(WP0_FB_BAD_SUBMISSION); return false;
    }
    cpu_cache_purge();
    scu_dma_transfer(0,(void *)VDP1_VRAM(0),commands,bytes);
    scu_dma_transfer_wait(0);
    /* VBlank may have expired during DMA. Never launch a late list. */
    uint32_t mask=cpu_intc_mask_get();
    cpu_intc_mask_set(15);
    if (wp0_framebuffer_status.error || phase!=PREPARING) {
        cpu_intc_mask_set(mask); return false;
    }
    scu_ic_status_set((scu_ic_status_reg_t)~SCU_IC_IST_SPRITE_END);
    draw_started=wp0_timer_count_get();
    wp0_framebuffer_status.submitted=1;
    phase=DRAWING;
    regs->ptmr=1;
    cpu_intc_mask_set(mask);
    return true;
}

void wp0_framebuffer_vblank_in(void)
{
    if (phase==OFF || phase==FAILED) return;
    registers_record();
    if (phase==PREPARING || phase==COMMITTED) {
        fail(WP0_FB_PREPARE_LATE); return;
    }
    if (phase==DRAWING) {
        draw_time_record();
        if (!(wp0_framebuffer_status.deadline_edsr&2U)) {
            fail(WP0_FB_DRAW_LATE); return;
        }
        /* CEF is authoritative if a low-priority SPRITE_END IRQ was delayed.
         * This timing is an upper bound; expose that fact in telemetry. */
        ++wp0_framebuffer_status.cef_fallback_count;
        wp0_framebuffer_status.draw_done=1;
    }
    /* Conservative coherence deadline: completed before Yaul's VBlank-IN
     * VDP2 DMA, earlier than VDP1's physical VBlank-OUT swap deadline. */
    phase=COMMITTED;
}

void wp0_framebuffer_stop(void)
{
    phase=OFF;
    regs->ptmr=0;
    regs->endr=0;
}

void wp0_framebuffer_release(void){
    wp0_framebuffer_stop();scu_dma_transfer_wait(0);
    if(!irq_owned)return;
    unsigned mask=cpu_intc_mask_get();cpu_intc_mask_set(15);
    scu_ic_mask_chg(SCU_IC_MASK_ALL,SCU_IC_MASK_SPRITE_END);
    scu_ic_status_set((scu_ic_status_reg_t)~SCU_IC_IST_SPRITE_END);
    scu_ic_ihr_set(SCU_IC_INTERRUPT_SPRITE_END,saved_sprite_end);
    scu_ic_mask_chg(SCU_IC_MASK_ALL&~SCU_IC_MASK_SPRITE_END,saved_sprite_mask);
    irq_owned=false;cpu_intc_mask_set(mask);
}
