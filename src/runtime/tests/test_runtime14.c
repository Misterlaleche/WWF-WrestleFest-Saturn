#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "wf_spatial.h"
#include "wf_gameplay_bindings.h"
static void active(wf_actor_raw_t*a,uint16_t x,uint16_t y){memset(a,0,sizeof(*a));wf_set_u8(a,WF_A_ACTIVE,0x80u);wf_set_be16(a,WF_A_X,x);wf_set_be16(a,WF_A_Y,y);wf_set_be16(a,WF_A_SPATIAL_CLASS,0x7777u);}
int main(void){wf_runtime_t rt;memset(&rt,0,sizeof(rt));active(&rt.actors[0],0x0239u,0x0142u);assert(wf_phase_spatial_2088e(&rt,0x02088eu,0)==WF_OK);assert(wf_be16(&rt.actors[0],WF_A_SPATIAL_CLASS)==1u);memset(&rt,0,sizeof(rt));active(&rt.actors[0],0x0200u,0x0080u);wf_set_u8(&rt.actors[0],WF_A_FLAGS33,4u);assert(wf_phase_spatial_2088e(&rt,0x02088eu,0)==WF_OK);assert(wf_be16(&rt.actors[0],WF_A_SPATIAL_CLASS)==1u);memset(&rt,0,sizeof(rt));active(&rt.actors[0],0x0200u,0x0110u);wf_set_u8(&rt.actors[0],WF_A_FLAGS32,1u);assert(wf_phase_spatial_2088e(&rt,0x02088eu,0)==WF_OK);assert(wf_be16(&rt.actors[0],WF_A_SPATIAL_CLASS)==0u);memset(&rt,0,sizeof(rt));active(&rt.actors[0],0x5000u,0xffffu);assert(wf_phase_spatial_2088e(&rt,0x02088eu,0)==WF_OK);assert(wf_be16(&rt.actors[0],WF_A_SPATIAL_CLASS)==0x7777u);puts("PASS test_runtime14");return 0;}
