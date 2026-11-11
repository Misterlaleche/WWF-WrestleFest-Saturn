#ifndef WF_FIXED_ATTRACT_STATE_H
#define WF_FIXED_ATTRACT_STATE_H
#include "wf_session.h"
#include "wf_board.h"
/* Sole admitted title diversion: completely quiescent original BOOT0724.
 * No ROM owner, game RAM, registers, RNG or credit is changed by this API. */
static inline wf_status_t wf_fixed_attract_title_enter(wf_session_t*s){
 if(!s||!s->initialized||s->phase!=WF_SESSION_FRONTEND)return WF_BAD_STATE;
 wf_r18_frontend_driver_t*d=&s->frontend;
 if(d->phase!=WF_R18_DRIVER_BOOT||d->pc!=0x724u||d->scene.boot.pc!=0x724u||
    !d->ram||d->ram_bytes<0x4000u||!d->regs.valid||d->irq_active||d->coin_active||
    d->active_owner||d->call_depth||d->fade.active||d->blocked_dependency||
    d->blocked_address||d->scene.boot.blocked_dependency)return WF_BAD_STATE;
 d->pc=d->scene.boot.pc=0x91cu;d->scene.boot.hold_return=0;++d->generation;
 return WF_OK;
}
typedef struct {wf_r18_frontend_driver_t*d;wf_board_t*b;uint32_t raster,vblank;} wf_fixed_irq_t;
static inline bool wf_fixed_attract_irq(void*u,unsigned level){
 wf_fixed_irq_t*c=u;
 if(level==3){++c->vblank;return wf_board_write(c->b,0x140000u,0,2);}
 ++c->raster;return wf_r18_frontend_driver_on_raster(c->d)==WF_OK;
}
/* Exactly the board's 17 raster edges per arcade tick. IRQ3 is acknowledged
 * but its original VBlank/START/renderer owner never runs in this lease. */
static inline bool wf_fixed_attract_raster_frame(wf_session_t*s,wf_board_t*b,
                                                uint32_t*raster,uint32_t*vblank){
 wf_fixed_irq_t ctx={&s->frontend,b,0,0};
 do{
  wf_board_advance(b);
  if(!wf_board_dispatch(b,s->frontend.control.sr,wf_fixed_attract_irq,&ctx))return false;
  if(b->irq2&&!b->irq3&&!wf_board_dispatch(b,s->frontend.control.sr,wf_fixed_attract_irq,&ctx))return false;
 }while(b->line<256u);
 if(raster)*raster+=ctx.raster;if(vblank)*vblank+=ctx.vblank;
 return true;
}
#endif
