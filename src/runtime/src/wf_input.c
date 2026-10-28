#include "wf_input.h"
/* ROM $F2D6, indexed by held direction nibble exactly; contradictory directions remain local. */
static const uint16_t dir_state1[16]={0x0000,0x0040,0x00C0,0x0000,0x0000,0x0020,0x00E0,0x0000,0x0080,0x0060,0x00A0,0x0000,0x0000,0x0000,0x0000,0x0000};
static uint16_t remap_controls(uint8_t active){
 uint16_t v=0;
 if(active&0x10u)v|=0x0001u;
 if(active&0x20u)v|=0x0002u;
 if(active&0x40u)v|=0x0004u;
 if(active&0x80u)v|=0x0100u;
 return v;
}
void wf_input_decode_514e(wf_actor_raw_t *a,uint8_t hardware_low,uint8_t pipeline_bit0){
 uint8_t active=(uint8_t)~hardware_low;
 /* Direction is current held physical nibble and lives in low byte of +A8/+A9. */
 uint16_t a8=(uint16_t)(active&0x0Fu); /* $5188 ANDI.W #$F clears upper byte. */
 wf_set_be16(a,WF_A_INPUT_DERIVED,a8);
 uint16_t newly=remap_controls(active);
 uint16_t effective;
 if(pipeline_bit0){
   uint16_t acc=(uint16_t)(wf_be16(a,WF_A_INPUT_ACC)|newly); /* +A4 */
   wf_set_be16(a,WF_A_INPUT_ACC,acc);
   effective=acc;
 }else{
   effective=wf_be16(a,WF_A_INPUT_ACC);
   wf_set_be16(a,WF_A_INPUT_ACC,0);
 }
 uint16_t prev=wf_be16(a,WF_A_INPUT_HELD);
 uint16_t fresh=(uint16_t)(effective & (uint16_t)~prev);
 wf_set_be16(a,WF_A_INPUT_FRESH,fresh); /* +A2 */
 wf_set_be16(a,WF_A_INPUT_HELD,effective); /* +A6 */
}
uint8_t wf_input_raw_action_lookup(uint8_t c,uint8_t cat,uint8_t mask){
 if(c>=12u||cat>=21u||mask<1u||mask>3u)return 0xFFu;
 return wf_action_matrix[c][cat][mask-1u];
}
uint16_t wf_input_direction_state1(uint8_t held){return dir_state1[held&0x0Fu];}
