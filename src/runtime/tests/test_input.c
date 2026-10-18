#include <assert.h>
#include <stdio.h>
#include "wf_input.h"
static void t_remap_accum(void){
 wf_actor_raw_t a={{0}};
 /* active-low: press B1 only => hardware low has bit4 cleared, all other bits high. */
 wf_input_decode_514e(&a,0xEF,1);
 assert(wf_be16(&a,WF_A_INPUT_ACC)==0x0001);
 assert(wf_be16(&a,WF_A_INPUT_FRESH)==0x0001);
 assert(wf_be16(&a,WF_A_INPUT_HELD)==0x0001);
 /* held B1 again: no new fresh edge */
 wf_input_decode_514e(&a,0xEF,1);
 assert(wf_be16(&a,WF_A_INPUT_FRESH)==0);
 /* B1 held + fresh B2: fresh mask must be 2, not 3 */
 wf_input_decode_514e(&a,0xCF,1);
 assert((wf_u8(&a,WF_A_INPUT_FRESH+1)&3u)==2u);
}
static void t_control_d(void){wf_actor_raw_t a={{0}};wf_input_decode_514e(&a,0x7F,1);assert(wf_be16(&a,WF_A_INPUT_FRESH)==0x0100);}
static void t_directions(void){static const unsigned x[16]={0,0x40,0xC0,0,0,0x20,0xE0,0,0x80,0x60,0xA0,0,0,0,0,0};for(unsigned i=0;i<16;i++)assert(wf_input_direction_state1(i)==x[i]);}
static void t_matrix(void){for(unsigned c=0;c<12;c++){for(unsigned cat=5;cat<=7;cat++)assert(wf_input_raw_action_lookup(c,cat,2)==0x48);for(unsigned m=1;m<=3;m++)assert(wf_input_raw_action_lookup(c,8,m)==0x29);assert(wf_input_raw_action_lookup(c,9,1)==0x15);assert(wf_input_raw_action_lookup(c,9,2)==0x17);assert(wf_input_raw_action_lookup(c,9,3)==0x15);for(unsigned m=1;m<=3;m++)assert(wf_input_raw_action_lookup(c,0x11,m)==0x1E);}}
int main(void){t_remap_accum();t_control_d();t_directions();t_matrix();puts("PASS test_input");}
