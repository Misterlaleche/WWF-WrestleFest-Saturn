#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "wf_runtime.h"
#include "wf_core_phases.h"
static wf_status_t ok(wf_runtime_t *r,uint32_t a,void *u){(void)r;(void)a;(void)u;return WF_OK;}
static void test_width(void){ wf_actor_raw_t a={{0}};wf_state4_write_word(&a,0x8105);wf_state4_write_low(&a,0x08);assert(wf_be16(&a,WF_A_STATE4)==0x8108);wf_state4_write_word(&a,0x0005);assert(wf_be16(&a,WF_A_STATE4)==0x0005);}
static void test_damage(void){wf_actor_raw_t a={{0}};wf_set_be16(&a,WF_A_STAMINA_MAX,120);wf_set_be16(&a,WF_A_STAMINA,30);wf_set_be16(&a,WF_A_PENDING_DAMAGE,7);wf_apply_pending_damage_and_tier(&a);assert(wf_be16(&a,WF_A_STAMINA)==23);assert(wf_be16(&a,WF_A_TIER)==2);assert(wf_be16(&a,WF_A_PENDING_DAMAGE)==0);}
static void test_branch_a(void){
 const uint32_t a[]={0x00514E,0x008B3A,0x02088E,0x020958,0x026936,0x02983C,0x00F560,0x00F18A,0x01C150,0x01F914,0x00F4C2,0x024062,0x024E58,0x00FDEE,0x010120,0x010E6A,0x007A00,0x0088A2,0x008710,0x0262D2,0x00BB14,0x007366,0x008CF8,0x009052,0x00899C,0x008ECC,0x002836,0x0018C4,0x0298EC};
 wf_phase_binding_t b[29];for(unsigned i=0;i<29;i++){b[i]=(wf_phase_binding_t){a[i],ok,0};}
 wf_runtime_t r;wf_runtime_init(&r,b,29);r.g.rng_state_valid=1u;r.g.rng_state=0x12345678u;r.g.credited_route=1u;
 assert(wf_runtime_frame(&r)==WF_BLOCKED);assert(r.trace_count==29);for(unsigned i=0;i<29;i++)assert(r.trace[i]==a[i]);assert(r.frame_ready==1u&&r.frame_stage==1u);assert((r.g.frame_handshake[1]&1u)!=0u);
 wf_runtime_vblank_ack(&r);assert(wf_runtime_frame(&r)==WF_OK);assert(r.g.frame_counter==1u);assert(r.frame_stage==0u);
}
static void test_fail_closed(void){wf_runtime_t r;wf_runtime_init(&r,0,0);assert(wf_runtime_frame(&r)==WF_BLOCKED);assert(r.trace_count==0u);r.g.rng_state_valid=1u;assert(wf_runtime_frame(&r)==WF_BLOCKED);assert(r.trace_count==1u&&r.trace[0]==0x00514E);}
int main(void){test_width();test_damage();test_branch_a();test_fail_closed();puts("PASS test_core");return 0;}
