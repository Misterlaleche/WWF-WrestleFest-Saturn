#ifndef WF_OVERLAY_IRQ_H
#define WF_OVERLAY_IRQ_H
#include <yaul.h>
/* BIOS SCU handler slots survive replacing an executable. In particular the
 * original MAC wrappers must never remain installed after their pointer table
 * and code disappear. Call only after all transfers are drained and CPU/SCU
 * interrupts are masked. The destination CRT installs its own active handlers;
 * the BIOS clear operation leaves the other sources on its safe default. */
static inline void wf_overlay_irq_clear(void){
 for(unsigned vector=0x40u;vector<=0x4du;vector++)
  scu_ic_ihr_clear((scu_ic_interrupt_t)vector);
}
#endif
