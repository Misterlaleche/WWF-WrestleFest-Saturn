/* Test-only access to the real private planner. Reset through its public
 * frame owner, then compare the observable prefix and capacity behavior. */
#include WF_PRESENT_SOURCE
#include <string.h>
int plan_case(const int32_t*pieces,unsigned n,uint16_t*tiles,uint8_t*commands,unsigned*out){
 static wf_runtime_t r;static wf_r18_video_t v;
 if(wf_r18_present_batch(&r,NULL,0,&v)!=WF_OK)return -1;
 unsigned consumed=0;
 for(;consumed<n;consumed++){
  const int32_t*p=pieces+consumed*6u;
  if(!add_cmd((uint16_t)p[0],p[1],p[2],(unsigned)p[3],(unsigned)p[4],(unsigned)p[5]))break;
 }
 out[0]=consumed;out[1]=plan.tile_count;out[2]=plan.cmd_count;
 for(unsigned i=0;i<plan.tile_count;i++){tiles[i*2]=plan.tile_upload[i].source_tile;tiles[i*2+1]=plan.tile_upload[i].stream_slot;}
 for(unsigned i=0;i<plan.cmd_count;i++){
  wf_vdp1_sprite_cmd_t*c=&plan.cmd[i];uint8_t*p=commands+i*11u;
  p[0]=(uint8_t)((uint16_t)c->x>>8);p[1]=(uint8_t)c->x;p[2]=(uint8_t)((uint16_t)c->y>>8);p[3]=(uint8_t)c->y;
  p[4]=(uint8_t)(c->stream_tile>>8);p[5]=(uint8_t)c->stream_tile;p[6]=c->cram_slot;p[7]=c->flip_x;p[8]=c->flip_y;p[9]=c->chain_index;p[10]=c->chain;
 }
 return 0;
}
