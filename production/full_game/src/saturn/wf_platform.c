#include "wf_platform.h"
#include "wf_cart.h"
#include <dram-cart.h>
volatile uint32_t wf_fields,wf_error,wf_cart_id,wf_cart_bytes,wf_cart_valid;
volatile uint32_t wf_logic_fields,wf_logic_ticks,wf_logic_pending,wf_logic_sequence;
static wf_arcade_clock_t clock;
static uint32_t last_field;
static void vblank_out(void *unused __unused){++wf_fields;smpc_peripheral_intback_issue();}
static uint32_t bus_read(void *p __unused,uint32_t at){return *(volatile uint32_t *)(uintptr_t)at;}
static void bus_write(void *p __unused,uint32_t at,uint32_t v){*(volatile uint32_t *)(uintptr_t)at=v;}
void wf_platform_init(void){
 smpc_peripheral_init();vdp_sync_vblank_out_set(vblank_out,NULL);
 dram_cart_init();wf_cart_id=dram_cart_id_get();wf_cart_bytes=dram_cart_size_get();
 const wf_cart_bus_t bus={bus_read,bus_write,NULL};
 void *cart_base=dram_cart_area_get();
 wf_cart_valid=wf_cart_validate(wf_cart_id,wf_cart_bytes,(uint32_t)(uintptr_t)cart_base,&bus);
}
void wf_platform_scene_begin(uint32_t phase){
 ++wf_logic_sequence;
 if(!wf_arcade_clock_init(&clock,phase)){++wf_logic_sequence;wf_platform_fail(1);return;}
 last_field=wf_fields;wf_logic_fields=wf_logic_ticks=wf_logic_pending=0;
 ++wf_logic_sequence;
}
static void field_wait(void){vdp2_sync();vdp2_sync_wait();}
bool wf_platform_tick_wait(void){
 /* One present opportunity per call; never update multiple visible poses in a burst.
  * Count every physical field elapsed during drawing, including missed deadlines. */
 for(;;){
  field_wait();
  uint32_t now=wf_fields,elapsed=now-last_field;last_field=now;
  ++wf_logic_sequence;
  while(elapsed--)if(!wf_arcade_clock_field(&clock)){wf_platform_fail(2);break;}
  bool ready=wf_arcade_clock_take(&clock);
  wf_logic_fields=(uint32_t)clock.fields;
  wf_logic_ticks=(uint32_t)clock.ticks;wf_logic_pending=clock.pending;
  ++wf_logic_sequence;
  if(clock.failed){wf_platform_fail(3);return false;}
  if(ready)return true;
 }
}
void wf_platform_hold_fields(uint32_t count){uint32_t start=wf_fields;while(wf_fields-start<count)field_wait();}
void wf_platform_pad(smpc_peripheral_digital_t *pad){smpc_peripheral_process();smpc_peripheral_digital_port(1,pad);}
void wf_platform_fail(uint32_t error){wf_error=error;vdp2_scrn_back_color_set(VDP2_VRAM_ADDR(3,0x1FFFE),RGB1555(1,31,0,0));}
