/* Catch stores that update the board shadow while leaving live ring cells
 * unchanged, or that lose a partial store after camera/animation transport. */
#include "wf_r18_video.h"
#include "wf_r18_ring.h"
#include <stdio.h>
#include <string.h>

#define CHECK(condition) do {if(!(condition)){fprintf(stderr,"FAIL %d: %s\n",__LINE__,#condition);return 1;}}while(0)
static uint8_t pack[114918];
static wf_r18_video_t video,snapshot;
static uint16_t names[2][1024],baseline[2][1024];
static unsigned be16(const uint8_t *p){return (p[0]<<8)|p[1];}
static unsigned be32(const uint8_t *p){return (be16(p)<<16)|be16(p+2);}
static unsigned source(unsigned dense){return be16(pack+be32(pack+16)+be16(pack+10)*128u+dense*2u);}
static unsigned dense(unsigned id){
 for(unsigned i=0;i<be16(pack+10);i++)if(source(i)==id)return i;
 return 65535u;
}
static int refresh(unsigned layer,int x,int y){return wf_r18_ring_cells_update(layer,x,y,names[layer],NULL);}
static int pixels(unsigned layer,unsigned cell,unsigned attr,unsigned id){
 unsigned tile=dense(id);if(tile==65535u)return 0;
 unsigned tx=cell&31u,ty=cell>>5;
 for(unsigned y=0;y<16;y++)for(unsigned x=0;x<16;x++){
  unsigned px=(attr&64u)?15u-x:x,py=(attr&128u)?15u-y:y;
  uint8_t byte=pack[be32(pack+16)+tile*128u+py*8u+px/2u];
  unsigned want=((attr&15u)<<4)|((px&1u)?byte&15u:byte>>4);
  uint8_t actual;
  if(wf_r18_ring_pixel(layer,(int)(tx*16u+x),(int)(ty*16u+y),&actual)||actual!=want)return 0;
 }
 return 1;
}
int main(int argc,char **argv){
 CHECK(argc==2);FILE *f=fopen(argv[1],"rb");CHECK(f&&fread(pack,1,sizeof pack,f)==sizeof pack);fclose(f);
 CHECK(!wf_r18_ring_bind(pack,sizeof pack));
 for(unsigned l=0;l<2;l++){CHECK(!refresh(l,0,0));memcpy(baseline[l],names[l],sizeof names[l]);}
 /* Bootstrap clears are RAM initialization; the later ROM map builder owns
  * the ring. Activating the bridge must not replay those clears as tiles. */
 wf_r17_fg0_write_t clear[]={{0x80000u,0u,4u},{0x82000u,0u,4u}};
 CHECK(wf_r18_video_batch(NULL,clear,2,&video)==WF_OK);
 wf_r18_video_ring_bind(&video);
 for(unsigned l=0;l<2;l++){CHECK(!refresh(l,0,0));CHECK(!memcmp(names[l],baseline[l],sizeof names[l]));}
 unsigned id0=source(0),id1=source(1),id2=source(2);
 /* Mixed widths and overlaps use the complete final batch value, across
  * both vectors. A transient invalid tile is not a final invalid tile. */
 wf_r17_fg0_write_t a[]={{0x80004u,0x00c30000u|id1,4u},{0x80006u,0xffffu,2u},
                           {0x82002u,0xa000u|id1,2u}};
 wf_r17_fg0_write_t b[]={{0x80006u,id2,2u},{0x80005u,0xc7u,1u},
                           {0x82003u,id2&255u,1u},{0x82002u,0xb0u|(id2>>8),1u}};
 CHECK(wf_r18_video_commit(NULL,a,3,b,4,&video)==WF_OK);
 CHECK(!refresh(0,0,0)&&!refresh(1,0,0));
 CHECK(names[0][1]==(0x7c00u|2u)&&names[1][1]==(0xb000u|2u));
 CHECK(pixels(0,1,0xc7u,id2)&&pixels(1,1,11u,id2));
 /* A write straddling two BG cells must update both final source words. */
 wf_r17_fg0_write_t cross={0x82005u,((id1&255u)<<24)|((0xc000u|id2)<<8)|(id0>>8),4u};
 wf_r17_fg0_write_t seed[]={{0x82004u,id1,2u},{0x82008u,id0,2u}};
 CHECK(wf_r18_video_batch(NULL,seed,2,&video)==WF_OK);
 CHECK(wf_r18_video_batch(NULL,&cross,1,&video)==WF_OK);
 CHECK(!refresh(1,0,0));CHECK(names[1][2]==1u&&names[1][3]==(0xc000u|2u)&&names[1][4]==0u);
 /* Prevalidation must reject the entire batch before HUD, palette, board
  * shadow, dirty flags or any visible cell can be changed. */
 unsigned missing=0;while(dense(missing)!=65535u)missing++;
 snapshot=video;uint16_t before=names[1][1];
 wf_r17_fg0_write_t invalid[]={{0xc0001u,0xa5u,1u},{0x180002u,0x1234u,2u},
                                 {0x82002u,missing,2u}};
 CHECK(wf_r18_video_batch(NULL,invalid,3,&video)==WF_BLOCKED);
 snapshot.failed_address=0x82002u;
 CHECK(!memcmp(&video,&snapshot,sizeof video));CHECK(!refresh(1,0,0)&&names[1][1]==before);
 /* Streaming removes an overlay in IRQ order. A following partial store
  * must preserve the streamed tile, not the stale bootstrap/owner shadow. */
 wf_r18_ring_tile_restore(1,1);CHECK(!refresh(1,0,0));CHECK(names[1][1]==baseline[1][1]);
 unsigned canonical=source(baseline[1][1]&1023u);
 wf_r17_fg0_write_t partial={0x82002u,0xd0u|(canonical>>8),1u};
 CHECK(wf_r18_video_batch(NULL,&partial,1,&video)==WF_OK);CHECK(!refresh(1,0,0));
 CHECK(names[1][1]==(0xd000u|(baseline[1][1]&1023u)));
 CHECK(pixels(1,1,13u,canonical));
 CHECK(!wf_r18_ring_tile_write(0,1,0x82u,(uint16_t)id1));
 partial=(wf_r17_fg0_write_t){0x80005u,0x45u,1u};
 CHECK(wf_r18_video_batch(NULL,&partial,1,&video)==WF_OK);CHECK(!refresh(0,0,0));
 CHECK(names[0][1]==(0x5400u|1u)&&pixels(0,1,0x45u,id1));
 /* Camera map changes and the IRQ's explicit restore retain authority. */
 CHECK(!refresh(1,512,512));CHECK(!refresh(1,0,0));
 CHECK(names[1][1]==(0xd000u|(baseline[1][1]&1023u)));
 wf_r18_ring_tile_restore(1,1);CHECK(!refresh(1,0,0));CHECK(names[1][1]==baseline[1][1]);
 puts("PASS live ring background: bootstrap, byte/word/long, overlaps, atomic invalid batches, pixels, camera and IRQ restore");
 return 0;
}
