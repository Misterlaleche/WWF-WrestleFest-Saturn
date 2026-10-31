#include "wf_platform.h"
#include "wf_cart.h"
#include "wf_cd.h"
#include "wf_pacer.h"
#include <dram-cart.h>
volatile uint32_t wf_fields,wf_error,wf_cart_id,wf_cart_bytes,wf_cart_valid;
volatile uint32_t wf_logic_fields,wf_logic_ticks,wf_logic_pending,wf_logic_sequence;
static wf_pacer_t pacer;
static bool started;
volatile uint32_t wf_load_fields,wf_load_depth,wf_clock_origin;
static void clock_publish(void){
 wf_logic_fields=(uint32_t)pacer.clock.fields;wf_logic_ticks=(uint32_t)pacer.clock.ticks;
 wf_logic_pending=pacer.clock.pending;wf_load_fields=(uint32_t)pacer.load_fields;wf_load_depth=pacer.load_depth;
}
static void vblank_out(void *unused __unused){++wf_fields;smpc_peripheral_intback_issue();}
static uint32_t bus_read(void *p __unused,uint32_t at){return *(volatile uint32_t *)(uintptr_t)at;}
static void bus_write(void *p __unused,uint32_t at,uint32_t v){*(volatile uint32_t *)(uintptr_t)at=v;}
void wf_platform_init(void){
 cpu_frt_init(CPU_FRT_CLOCK_DIV_32);
 smpc_peripheral_init();vdp_sync_vblank_out_set(vblank_out,NULL);
 dram_cart_init();wf_cart_id=dram_cart_id_get();wf_cart_bytes=dram_cart_size_get();
 const wf_cart_bus_t bus={bus_read,bus_write,NULL};
 void *cart_base=dram_cart_area_get();
 wf_cart_valid=wf_cart_validate(wf_cart_id,wf_cart_bytes,(uint32_t)(uintptr_t)cart_base,&bus);
 if(!wf_cd_init())wf_platform_fail(20);
}
void wf_platform_scene_begin(uint32_t phase){
 ++wf_logic_sequence;
 if(!wf_pacer_init(&pacer,wf_fields,phase)){++wf_logic_sequence;wf_platform_fail(1);return;}
 wf_clock_origin=pacer.last_field;started=true;clock_publish();
 ++wf_logic_sequence;
}
static void field_wait(void){vdp2_sync();vdp2_sync_wait();}
bool wf_platform_load(bool begin){
 if(!started)return true; /* RESET precedes the original ready boundary. */
 ++wf_logic_sequence;
 bool ok=begin?wf_pacer_load_begin(&pacer,wf_fields):wf_pacer_load_end(&pacer,wf_fields);
 clock_publish();++wf_logic_sequence;
 if(!ok)wf_platform_fail(4);return ok;
}
bool wf_platform_tick_wait(void){
 if(!started){wf_platform_fail(5);return false;}
 /* At most one visible tick per call. Active overruns retain their debt. */
 for(;;){
  if(wf_fields==pacer.last_field)field_wait();
  ++wf_logic_sequence;
  bool ok=wf_pacer_account(&pacer,wf_fields),ready=ok&&wf_pacer_take(&pacer);
  clock_publish();++wf_logic_sequence;
  if(pacer.clock.failed){wf_platform_fail(3);return false;}
  if(ready)return true;
 }
}
void wf_platform_hold_fields(uint32_t count){uint32_t start=wf_fields;while(wf_fields-start<count)field_wait();}
void wf_platform_pad(smpc_peripheral_digital_t *pad){smpc_peripheral_process();smpc_peripheral_digital_port(1,pad);}
void wf_platform_fail(uint32_t error){wf_error=error;vdp2_scrn_display_set(0);vdp2_tvmd_display_set();vdp2_scrn_back_color_set(VDP2_VRAM_ADDR(3,0x1FFFE),RGB1555(1,31,0,0));}
