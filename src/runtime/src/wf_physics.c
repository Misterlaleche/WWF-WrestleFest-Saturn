#include "wf_physics.h"
static int32_t s32(uint32_t x){return (int32_t)x;}
static uint32_t u32(int32_t x){return (uint32_t)x;}
static int16_t s16(uint16_t x){return (int16_t)x;}
static void add_long(wf_actor_raw_t *a,size_t off,int32_t d){wf_set_be32(a,off,u32(s32(wf_be32(a,off))+d));}
static void coeffs(uint8_t angle,int16_t *cx,int16_t *cy){
 uint8_t q=(uint8_t)(angle>>6),i;
 switch(q){
   case 0:i=(uint8_t)(angle&0x3Fu);*cx=(int16_t)wf_qx[i];*cy=(int16_t)wf_qy[i];break;
   case 1:i=(uint8_t)(0x40u-(angle&0x3Fu));*cx=(int16_t)wf_qx[i];*cy=-(int16_t)wf_qy[i];break;
   case 2:i=(uint8_t)(angle&0x3Fu);*cx=-(int16_t)wf_qx[i];*cy=-(int16_t)wf_qy[i];break;
   default:i=(uint8_t)(0x40u-(angle&0x3Fu));*cx=-(int16_t)wf_qx[i];*cy=(int16_t)wf_qy[i];break;
 }
}
void wf_physics_integrate_common(wf_actor_raw_t *a){
 uint8_t mode=wf_u8(a,WF_A_MOTION_MODE);
 if(mode==0)return;
 if(mode==1){
   uint8_t angle=wf_u8(a,WF_A_ANGLE); uint8_t speed=wf_u8(a,WF_A_MAGNITUDE+1u); int16_t x,y; coeffs(angle,&x,&y);
   add_long(a,WF_A_X,(int32_t)x*(uint32_t)speed); add_long(a,WF_A_Y,(int32_t)y*(uint32_t)speed); return;
 }
 if(mode==2){
   add_long(a,WF_A_X,(int32_t)s16(wf_be16(a,WF_A_VX))*256);
   add_long(a,WF_A_Y,(int32_t)s16(wf_be16(a,WF_A_VY))*256);
   add_long(a,WF_A_Z,(int32_t)s16(wf_be16(a,WF_A_VZ))*256);
   wf_set_be16(a,WF_A_VZ,(uint16_t)(wf_be16(a,WF_A_VZ)-wf_be16(a,WF_A_VZ_DEC)));
 }
}
uint8_t wf_physics_walk_speed(uint8_t c,uint8_t f){if(c>=12u)return 0;return (f&0x08u)?wf_walk_alt_speed[c]:wf_walk_speed[c];}
uint8_t wf_physics_run_speed(uint8_t c){return c<12u?wf_run_speed[c]:0;}
