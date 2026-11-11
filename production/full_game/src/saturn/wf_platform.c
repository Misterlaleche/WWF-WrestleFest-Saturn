#include "wf_platform.h"
#include "wf_cart.h"
#include "wf_cd.h"
#include "wf_pacer.h"
#ifdef WF_ALPHA_RETURN
#include "wf_combat_loader.h"
#endif
#include <dram-cart.h>
#if defined(WF_COMBAT_TAG_TRANSPORT) || defined(WF_CD_RATE_GUARDED)
#include "wp2015_cd_rate.h"
#endif
#ifdef WF_COMBAT_BANKED_TRANSPORT
#include "wf_saturn_yaul_backend.h"
#include "wp2015_vdp1_observer.h"
#endif
#ifdef WF_COMBAT_MODULE
#include "wp2015_vdp1_observer.h"
#endif
volatile uint32_t wf_fields,wf_error,wf_cart_id,wf_cart_bytes,wf_cart_valid;
volatile uint32_t wf_logic_fields,wf_logic_ticks,wf_logic_pending,wf_logic_sequence;
static wf_pacer_t pacer;
static bool started;
volatile uint32_t wf_load_fields,wf_load_depth,wf_clock_origin;
static void clock_publish(void){
 wf_logic_fields=(uint32_t)pacer.clock.fields;wf_logic_ticks=(uint32_t)pacer.clock.ticks;
 wf_logic_pending=pacer.clock.pending;wf_load_fields=(uint32_t)pacer.load_fields;wf_load_depth=pacer.load_depth;
}
static void vblank_out(void *unused __unused){
#ifdef WF_COMBAT_BANKED_TRANSPORT
 wf_saturn_yaul_vblank_out();
#endif
#if defined(WF_COMBAT_TAG_TRANSPORT) || defined(WF_COMBAT_MODULE)
 wp2015_vdp1_observe_vblank_out();
#endif
 ++wf_fields;smpc_peripheral_intback_issue();
}
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
bool wf_platform_presentation_tick_wait(void){
 if(!started){wf_platform_fail(5);return false;}
 /* Presentation follows fresh rational tick fields without spending the
  * inherited combat debt. Accounting/publication still use the same clock. */
 for(;;){
  if(wf_fields==pacer.last_field)field_wait();
  ++wf_logic_sequence;
  bool ready=wf_pacer_presentation_step(&pacer,wf_fields);
  clock_publish();++wf_logic_sequence;
  if(pacer.clock.failed){wf_platform_fail(3);return false;}
  if(ready)return true;
 }
}
void wf_platform_hold_fields(uint32_t count){uint32_t start=wf_fields;while(wf_fields-start<count)field_wait();}
void wf_platform_pad(smpc_peripheral_digital_t *pad){smpc_peripheral_process();smpc_peripheral_digital_port(1,pad);}
void wf_platform_fail(uint32_t error){
 extern void wf_failure_screen_show(uint32_t);
 if(!wf_error)wf_error=error;
#ifdef WF_ALPHA_RETURN
 /* A completed CRT explicitly owns this diagnostic. While mailbox2/5 is
  * replacing code, instruction bytes may match before BSS has been cleared;
  * an observer must never infer a fatal state from those old BSS bytes. */
 WF_COMBAT_MAILBOX[0]=
#ifdef WF_COMBAT_MODULE
  WF_COMBAT_FATAL_MARKER;
#else
  WF_FRONTEND_FATAL_MARKER;
#endif
#endif
 wf_failure_screen_show(wf_error);
}
bool wf_platform_clock_export(wf_pacer_t*out){
 if(!out||!started||!pacer.load_depth)return false;
 ++wf_logic_sequence;bool ok=wf_pacer_account(&pacer,wf_fields);*out=pacer;
 clock_publish();++wf_logic_sequence;return ok;
}
bool wf_platform_clock_restore(const wf_pacer_t*in,uint32_t fields,uint32_t origin){
 if(!in||in->clock.failed||!in->load_depth||in->last_field!=fields)return false;
#if defined(WF_COMBAT_TAG_TRANSPORT) || defined(WF_CD_RATE_GUARDED)
 /* Adding the saved origin is not elapsed physical time. Drain before the
  * CD history can observe this discontinuity and incorrectly age commands. */
 int rate_rc=wp2015_cd_rate_before_frt_reset();
 if(rate_rc){wf_cd_error=(uint32_t)rate_rc;return false;}
#endif
 unsigned mask=cpu_intc_mask_get();cpu_intc_mask_set(15);
 ++wf_logic_sequence;pacer=*in;wf_fields+=fields;wf_clock_origin=origin;started=true;
 bool ok=wf_pacer_account(&pacer,wf_fields);clock_publish();++wf_logic_sequence;
 cpu_intc_mask_set(mask);return ok;
}

#if defined(WF_FIXED_ATTRACT) && WF_FIXED_ATTRACT
volatile uint32_t wf_attract_elapsed_fields;
static bool attract_paused;
static uint32_t attract_pause_start;
void wf_platform_vblank_out(void){vblank_out(NULL);}
void wf_platform_video_callbacks_restore(void){
 vdp_sync_vblank_out_set(vblank_out,NULL);vdp_sync_vblank_in_set(NULL,NULL);
}
bool wf_platform_attract_pause(bool begin){
 if(!started||begin==attract_paused)return false;
 ++wf_logic_sequence;
 const uint32_t now=wf_fields; /* One boundary even if VBlank lands during accounting. */
 if(begin){
  if(!wf_pacer_account(&pacer,now)){++wf_logic_sequence;return false;}
  attract_pause_start=now;attract_paused=true;
 }else{
  uint32_t elapsed=now-attract_pause_start;
  pacer.last_field+=elapsed;wf_attract_elapsed_fields+=elapsed;
  attract_paused=false;
 }
 clock_publish();++wf_logic_sequence;return true;
}
#endif
