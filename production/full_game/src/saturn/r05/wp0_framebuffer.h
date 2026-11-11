#ifndef WP0_FRAMEBUFFER_H
#define WP0_FRAMEBUFFER_H
#include <stdbool.h>
#include <stdint.h>

enum {
    WP0_FB_PREPARE_LATE=401,
    WP0_FB_DRAW_LATE=402,
    WP0_FB_BAD_SUBMISSION=403
};
typedef struct {
    uint32_t error,last_draw_ticks,max_draw_ticks,draw_irq_count;
    uint32_t cef_fallback_count,deadline_edsr,deadline_modr,deadline_copr;
    uint32_t submitted,draw_done;
} wp0_framebuffer_status_t;
extern volatile wp0_framebuffer_status_t wp0_framebuffer_status;

/* Owns VDP1 registers and SPRITE_END. Do not also use Yaul vdp1_sync*. */
void wp0_framebuffer_initialize(void);
bool wp0_framebuffer_begin(void);
bool wp0_framebuffer_submit(const void *commands,unsigned bytes);
/* Called before Yaul commits VDP2 registers and queued VRAM/CRAM writes. */
void wp0_framebuffer_vblank_in(void);
void wp0_framebuffer_stop(void);
void wp0_framebuffer_release(void);
#endif
