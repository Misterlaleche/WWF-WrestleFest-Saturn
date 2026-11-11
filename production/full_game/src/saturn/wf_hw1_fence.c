#include "wf_hw1_fence.h"
#include <yaul.h>
static bool published,held;
volatile uint32_t wf_hw1_fence_waits,wf_hw1_fence_timeouts;
volatile uint32_t wf_hw1_fence_checks,wf_hw1_fence_violations,wf_hw1_fence_idle_clears;
bool wf_hw1_fence_begin(void){
 volatile vdp1_ioregs_t *const regs=(volatile vdp1_ioregs_t*)VDP1_IOREG_BASE;
 /* Yaul AUTO's DMA callback enables PTMR=2. Its VBlank ACK can precede
  * the current END, and the same list starts again at the next swap.
  * Disable future starts before waiting for the current reader. Upload
  * completion will re-enable AUTO through the original SDK callback. */
 held=false;regs->ptmr=0;
 if(!published){held=true;return true;}
 uint16_t begin=cpu_frt_count_get();bool waited=false;
 while(!(regs->edsr&2u)){
  if(!waited){++wf_hw1_fence_waits;waited=true;}
  if((uint16_t)(cpu_frt_count_get()-begin)>=0x8000u){++wf_hw1_fence_timeouts;return false;}
 }
 held=true;return true;
}
bool wf_hw1_fence_check(void){
 volatile vdp1_ioregs_t *const regs=(volatile vdp1_ioregs_t*)VDP1_IOREG_BASE;
 ++wf_hw1_fence_checks;
 if(!held||(regs->modr&0x100u)){++wf_hw1_fence_violations;return false;}
 /* CEF also clears at a framebuffer swap while PTMR is IDLE. That is
  * distinct from a reader: begin() already observed END and no command
  * has re-enabled automatic drawing during this ownership interval. */
 if(!(regs->edsr&2u))++wf_hw1_fence_idle_clears;
 return true;
}
void wf_hw1_fence_publish(void){held=false;published=true;}
