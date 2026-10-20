#include "wf_r18_world.h"
#include "wf_r18_ring.h"
#include <string.h>
static uint16_t be16(const uint8_t*p){return (uint16_t)((p[0]<<8)|p[1]);}
static uint32_t be32(const uint8_t*p){return ((uint32_t)p[0]<<24)|((uint32_t)p[1]<<16)|(p[2]<<8)|p[3];}
static wf_status_t read(wf_r18_world_ctx_t*c,uint32_t at,void*dst,size_t n){if(!c||!c->read_rom||at>0x80000u||n>0x80000u-at)return WF_BLOCKED;return c->read_rom(c->read_user,at,dst,n);}
static wf_status_t ptr(wf_r18_world_ctx_t*c,uint32_t at,uint32_t*out){uint8_t b[4];wf_status_t s=read(c,at,b,4);if(s==WF_OK)*out=be32(b);return s;}
wf_status_t wf_r18_world_288f0(wf_runtime_t *rt,uint8_t record[4],uint8_t bank,void *user){
 wf_r18_world_ctx_t*c=user;uint32_t table,handler,desc;uint8_t r[4],header[2],frame[2];unsigned id;
 if(!rt||!record||!c||!rt->m68k_regs.valid||bank>=7||rt->g.global_007e>=7)return WF_BLOCKED;
 memcpy(r,record,4);id=r[0]&63;
 if(ptr(c,0x28916+bank*4,&table)!=WF_OK||ptr(c,table+id*4,&handler)!=WF_OK||handler!=0x28b3e)return WF_BLOCKED;
 int first=!(r[0]&64);r[0]|=64;
 if(first||r[2]==0){
  if(ptr(c,0x28c66+rt->g.global_007e*4,&table)!=WF_OK||ptr(c,table+id*4,&desc)!=WF_OK||read(c,desc,header,2)!=WF_OK)return WF_BLOCKED;
  if(first){
   r[3]=header[1];int found=0;
   if(r[3])for(unsigned j=0;j<46;j++){uint8_t *other=rt->branchb_objs_188a[j];if(other!=record&&(other[3]&63)==(r[3]&63)){r[1]=other[1];r[2]=other[2];found=1;break;}}
   if(!found){r[1]=0;if(read(c,desc+3,&r[2],1)!=WF_OK)return WF_BLOCKED;}
  }else{r[1]++;if(header[0]<r[1])r[1]=0;if(read(c,desc+3+r[1]*2,&r[2],1)!=WF_OK)return WF_BLOCKED;}
  if(read(c,desc+2+r[1]*2,frame,2)!=WF_OK)return WF_BLOCKED;
  unsigned layer=(r[3]&128)?1:0,n=c->animation_count[layer];if(n>=48)return WF_BLOCKED;
  c->animation_queue[layer][n]=frame[0];c->animation_count[layer]=(uint16_t)(n+1);
 }else r[2]--;
 memcpy(record,r,4);
 rt->m68k_regs.d[1]=(rt->m68k_regs.d[1]&0xffff0000u)|(id<<2);
 return WF_OK;
}
wf_status_t wf_r18_world_26b68(wf_runtime_t *rt,uint8_t side,uint16_t direction,void *user){
 wf_r18_world_ctx_t*c=user;wf_r18_world_transport_t cache[16];uint32_t root,offsets;uint8_t coords[32];
 if(!rt||!c||side>1||direction>3||rt->g.global_007e>=8)return WF_BLOCKED;
 if(ptr(c,(side?0x270bc:0x2709c)+rt->g.global_007e*4,&root)!=WF_OK||ptr(c,0x26f82+direction*4,&offsets)!=WF_OK||read(c,offsets,coords,32)!=WF_OK)return WF_BLOCKED;
 for(unsigned i=0;i<16;i++){
  uint16_t x=(uint16_t)(rt->g.global_17e2+(int8_t)coords[i*2]);
  uint16_t y=(uint16_t)(0x40u-(uint16_t)(rt->g.global_17e4+(int8_t)coords[i*2+1]));
  uint16_t off=(uint16_t)(((x/10)*4)+((y>>3)*32));uint32_t page;
  if(ptr(c,(uint32_t)(root+(int16_t)off),&page)!=WF_OK)return WF_BLOCKED;
  cache[i].source=page+(x%10)*(side?4:8)+(y&7)*(side?0x50:0xa0);
  cache[i].destination=(side?0x82000:0x80000)+((x+16)&15)*(side?4:8)+(y&15)*(side?0x80:0x100);
 }
 memcpy(c->cache[side],cache,sizeof cache);return WF_OK;
}
wf_status_t wf_r18_world_commit(wf_runtime_t *rt,wf_r18_world_ctx_t*c){
 if(!rt||!c||rt->g.global_007e>=7)return WF_BLOCKED;
 for(unsigned layer=0;layer<2;layer++){
  unsigned flag=layer?0x4000:0x8000;
  if(rt->g.global_16d2&flag){
   for(unsigned i=0;i<16;i++){
    uint32_t off=c->cache[layer][i].destination-(layer?0x82000:0x80000);unsigned cell=off/(layer?2:4);
    if(cell+33>=1024)return WF_BLOCKED;
    wf_r18_ring_tile_restore(layer,cell);wf_r18_ring_tile_restore(layer,cell+1);wf_r18_ring_tile_restore(layer,cell+32);wf_r18_ring_tile_restore(layer,cell+33);
   }
   rt->g.global_16d2=(uint16_t)(rt->g.global_16d2&~flag);
  }
 }
 for(unsigned layer=0;layer<2;layer++){
  if(!c->animation_count[layer])continue;
  uint32_t destinations,source,table=(layer?0x295dc:0x295a4)+rt->g.global_007e*8;
  if(ptr(c,table,&destinations)!=WF_OK||ptr(c,table+4,&source)!=WF_OK)return WF_BLOCKED;
  for(unsigned i=0;i<c->animation_count[layer];i++){
   unsigned frame=c->animation_queue[layer][i];uint8_t address[2],pixels[64];
   if(read(c,destinations+frame*2,address,2)!=WF_OK||read(c,source+frame*(layer?32:64),pixels,layer?32:64)!=WF_OK)return WF_BLOCKED;
   unsigned first=be16(address)/(layer?2:4);
   for(unsigned y=0;y<4;y++)for(unsigned x=0;x<4;x++){
    unsigned at=(y*4+x)*(layer?2:4);uint16_t attr=be16(pixels+at),tile;
    if(layer){tile=attr&4095;attr>>=12;}else tile=be16(pixels+at+2)&8191;
    if(wf_r18_ring_tile_write(layer,first+y*32+x,attr,tile))return WF_BLOCKED;
   }
  }
  c->animation_count[layer]=0;
 }
 return WF_OK;
}
