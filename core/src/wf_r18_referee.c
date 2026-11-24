#include "wf_r18_referee.h"

wf_status_t wf_r18_referee_direction_20c8(wf_runtime_t *rt,wf_actor_raw_t *actor,void *user){
 static const uint8_t base[4]={0,192,64,128}; /* ROM $2184. */
 uint16_t dx,dy;unsigned quadrant=0,sum,component;(void)user;
 if(!rt||!actor)return WF_BAD_STATE;
 dx=(uint16_t)(wf_be16(actor,0x60)-wf_be16(actor,WF_A_X));
 dy=(uint16_t)(wf_be16(actor,0x62)-wf_be16(actor,WF_A_Y));
 if(dx&0x8000u){quadrant|=1u;dx=(uint16_t)(0u-dx);}
 if(dy&0x8000u){quadrant|=2u;dy=(uint16_t)(0u-dy);}
 if(dx>255u)dx=255u;
 if(dy>255u)dy=255u;
 sum=(unsigned)dx+dy;component=(quadrant==0u||quadrant==3u)?dx:dy;
 uint8_t angle=(uint8_t)(base[quadrant]+component*64u/(sum?sum:1u));
 uint8_t *scratch=rt->g.direction_scratch_15e4;
 /* Caller $1FCC4 seeds pointer $1C11F4 and target X/Y. $20C8 replaces
  * byte 0 with the angle and words +2/+4 with saturated absolute deltas;
  * byte 1 of the pointer and target Y at +6 survive. */
 scratch[0]=angle;scratch[1]=0x1cu;
 scratch[2]=(uint8_t)(dx>>8);scratch[3]=(uint8_t)dx;
 scratch[4]=(uint8_t)(dy>>8);scratch[5]=(uint8_t)dy;
 scratch[6]=wf_u8(actor,0x62);scratch[7]=wf_u8(actor,0x63);
 wf_set_u8(actor,WF_A_ANGLE,angle);
 return WF_OK;
}

wf_status_t wf_r18_referee_number_2067c(wf_runtime_t *rt,uint16_t id,void *user){
 wf_r18_video_t *video=user;uint8_t ptr[4],glyph[16];wf_r17_fg0_write_t writes[32];
 unsigned number=id&255u,columns=number<10u?1u:2u;size_t count=0;uint32_t address,source;
 if(!rt||!video)return WF_BAD_STATE;
 if(number>20u||!video->read_rom)return WF_BLOCKED; /* ROM table $2073A has 21 entries. */
 address=(id&0x8000u)?0xc1188u:(number<10u?0xc0748u:0xc0744u);
 if(video->read_rom(video->read_user,0x2073au+number*4u,ptr,4)!=WF_OK)return WF_BLOCKED;
 source=((uint32_t)ptr[0]<<24)|((uint32_t)ptr[1]<<16)|((uint32_t)ptr[2]<<8)|ptr[3];
 if(video->read_rom(video->read_user,source,glyph,columns*8u)!=WF_OK)return WF_BLOCKED;
 for(unsigned column=0;column<columns;column++)for(unsigned row=0;row<4;row++){
  uint32_t at=address+column*8u+row*256u;unsigned off=column*8u+row*2u;
  writes[count++]=(wf_r17_fg0_write_t){at,glyph[off],2};
  writes[count++]=(wf_r17_fg0_write_t){at+2u,0xeau,2};
  writes[count++]=(wf_r17_fg0_write_t){at+4u,glyph[off+1u],2};
  writes[count++]=(wf_r17_fg0_write_t){at+6u,0xeau,2};
 }
 return wf_r18_video_batch(rt,writes,count,video);
}
wf_status_t wf_r18_referee_display_2067c(wf_runtime_t *rt,void *user){
 if(!rt)return WF_BAD_STATE;
 return wf_r18_referee_number_2067c(rt,(uint16_t)(wf_be16(&rt->referee,0x24)>>1),user);
}
wf_status_t wf_r18_referee_clear_206fe(wf_runtime_t *rt,void *user){
 wf_r17_fg0_write_t writes[16];size_t count=0;
 if(!rt||!user)return WF_BAD_STATE;
 for(unsigned row=0;row<4;row++)for(unsigned column=0;column<4;column++)
  writes[count++]=(wf_r17_fg0_write_t){0xc0744u+row*256u+column*4u,0,4};
 return wf_r18_video_batch(rt,writes,count,user);
}
