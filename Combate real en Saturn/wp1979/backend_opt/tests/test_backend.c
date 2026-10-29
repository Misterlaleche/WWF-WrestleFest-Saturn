/* Tests the real Saturn branch of the backend. The reference rasterizer
 * consumes the original 16x16 pixels and never reads crop metadata. The
 * second rasterizer consumes hardware commands and copied VRAM only. */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#define WF_PLATFORM_SATURN 1
#ifndef BACKEND_SOURCE
#error BACKEND_SOURCE must identify the production backend copy under test
#endif
#include BACKEND_SOURCE
#ifndef EXPECTED_ATLAS_TILES
#define EXPECTED_ATLAS_TILES 6381u
#endif

#define CHECK(e) do { if(!(e)){fprintf(stderr,"FAIL %s:%d: %s (case %u)\n",__func__,__LINE__,#e,cases);exit(1);} }while(0)
uint8_t test_vram[0x80000];
static unsigned cases,frames,render_calls,put_calls,put_waits,sync_calls,sync_waits,dma_count,dma_waits;
static size_t dma_offsets[256],dma_sizes[256];
static vdp1_cmdt_t submitted[WF_VDP1_COMMANDS_MAX+4u];
static unsigned submitted_count,reader_calls;
static uint8_t source_tiles[8][128];
static uint8_t stage_storage[256*128+64] __attribute__((aligned(32)));
static uint8_t *const stage=stage_storage+32;
static wf_saturn_yaul_backend_t backend;

void scu_dma_transfer(unsigned channel,void*dst,const void*src,size_t bytes){
 CHECK(channel==0);CHECK(dma_count<256);
 CHECK((uint8_t*)dst>=test_vram+WF_SATURN_VDP1_DYNAMIC_OFFSET);
 CHECK((uint8_t*)dst+bytes<=test_vram+WF_SATURN_VDP1_DYNAMIC_OFFSET+256u*128u);
 CHECK((const uint8_t*)src>=stage);CHECK((const uint8_t*)src+bytes<=stage+256u*128u);
 CHECK(bytes&&!(bytes%128u));
 dma_offsets[dma_count]=(uint8_t*)dst-test_vram;dma_sizes[dma_count++]=bytes;
 memcpy(dst,src,bytes);
}
void scu_dma_transfer_wait(unsigned channel){CHECK(channel==0);++dma_waits;}
void vdp1_sync_cmdt_list_put(vdp1_cmdt_list_t*l,unsigned first){
 CHECK(first==0);CHECK(l->count<=WF_VDP1_COMMANDS_MAX+4u);
 memcpy(submitted,l->cmdts,l->count*sizeof(*submitted));submitted_count=l->count;++put_calls;
}
void vdp1_sync_put_wait(void){++put_waits;}
void vdp1_sync_render(void){++render_calls;}
void vdp1_sync(void){++sync_calls;}
void vdp1_sync_wait(void){++sync_waits;}
wf_status_t wf_saturn_render_submit(const wf_vdp1_frame_plan_t*p,const wf_vdp1_assets_t*a,const wf_saturn_render_ops_t*o,void*c){(void)p;(void)a;(void)o;(void)c;abort();}

static void reset_frame_observation(void){
 render_calls=put_calls=put_waits=sync_calls=sync_waits=dma_count=dma_waits=submitted_count=0;
 memset(submitted,0xcd,sizeof submitted);
}
static void init_backend(void){
 memset(stage_storage,0xa5,sizeof stage_storage);
 memset(test_vram+WF_SATURN_VDP1_DYNAMIC_OFFSET-32,0x6d,256u*128u+64);
 wf_saturn_yaul_backend_init(&backend,stage,256u*128u);backend.defer_sync=1;reader_calls=0;
 reset_frame_observation();
}
static void guards(void){
 for(unsigned i=0;i<32;i++)CHECK(stage_storage[i]==0xa5&&stage[256u*128u+i]==0xa5);
 for(unsigned i=0;i<32;i++)CHECK(test_vram[WF_SATURN_VDP1_DYNAMIC_OFFSET-32+i]==0x6d&&test_vram[WF_SATURN_VDP1_DYNAMIC_OFFSET+256u*128u+i]==0x6d);
}
static void begin(unsigned tiles,unsigned cmds){reset_frame_observation();CHECK(g_ops.begin_frame(&backend,tiles,0,cmds)==WF_OK);}
static void finish(void){
 CHECK(g_ops.end_frame(&backend)==WF_OK);CHECK(put_calls==1&&put_waits==1&&render_calls==1);
 CHECK(sync_calls==(backend.defer_sync?0u:1u)&&sync_waits==sync_calls);CHECK(dma_waits==dma_count);
 CHECK(submitted_count>=3);CHECK(submitted[0].cmd_ctrl==9&&submitted[1].cmd_ctrl==10);
 CHECK(submitted[submitted_count-1].cmd_ctrl==0x8000);guards();++frames;
}
static wf_status_t reader(uint16_t source,const uint8_t **p){++reader_calls;if(source>=8)return WF_BLOCKED;*p=source_tiles[source];return WF_OK;}
static void pixel(uint8_t *t,unsigned x,unsigned y,unsigned p){unsigned at=y*8+x/2;if(x&1)t[at]=(uint8_t)((t[at]&0xf0)|(p&15));else t[at]=(uint8_t)((t[at]&15)|((p&15)<<4));}
static unsigned pen(const uint8_t*t,unsigned width,unsigned x,unsigned y){unsigned b=t[y*(width/2)+x/2];return x&1?b&15:b>>4;}

enum { W=48,H=48 };
static uint8_t expected[W*H],observed[W*H];
static void ref_draw(const uint8_t*raw,const wf_vdp1_sprite_cmd_t*c){
 for(unsigned y=0;y<16;y++)for(unsigned x=0;x<16;x++){
  unsigned p=pen(raw,16,x,y);int dx=c->x+(c->flip_x?15-(int)x:(int)x),dy=c->y+(c->flip_y?15-(int)y:(int)y);
  if(p&&dx>=0&&dx<W&&dy>=0&&dy<H)expected[dy*W+dx]=(uint8_t)p;
 }
}
static void hw_draw(void){
 for(unsigned i=2;i+1<submitted_count;i++){
  const vdp1_cmdt_t*c=&submitted[i];CHECK(!(c->cmd_ctrl&0xffcf));CHECK(c->cmd_link==0);
  CHECK(c->cmd_pmod==0x80);CHECK(c->cmd_colr>=WF_SATURN_CRAM_SPRITE_FIRST);
  CHECK(!c->cmd_xb&&!c->cmd_yb&&!c->cmd_xc&&!c->cmd_yc&&!c->cmd_xd&&!c->cmd_yd&&!c->cmd_grda&&!c->reserved);
  unsigned width=((c->cmd_size>>8)&63)*8,height=c->cmd_size&255;size_t at=(size_t)c->cmd_srca*8;
  CHECK((width==8||width==16)&&height&&height<=16);CHECK(at>=WF_SATURN_VDP1_DYNAMIC_OFFSET);
  CHECK(at+width*height/2<=WF_SATURN_VDP1_DYNAMIC_OFFSET+256u*128u);
  for(unsigned y=0;y<height;y++)for(unsigned x=0;x<width;x++){
   unsigned p=pen(test_vram+at,width,x,y);int dx=c->cmd_xa+((c->cmd_ctrl&16)?(int)width-1-(int)x:(int)x),dy=c->cmd_ya+((c->cmd_ctrl&32)?(int)height-1-(int)y:(int)y);
   if(p&&dx>=0&&dx<W&&dy>=0&&dy<H)observed[dy*W+dx]=(uint8_t)p;
  }
 }
}
static void compare_one(const uint8_t*raw,const wf_vdp1_sprite_cmd_t*c){
 memset(expected,0x80,sizeof expected);memset(observed,0x80,sizeof observed);ref_draw(raw,c);hw_draw();CHECK(!memcmp(expected,observed,sizeof expected));
 unsigned left=0,right=0,top=16,bottom=0;
 for(unsigned y=0;y<16;y++)for(unsigned x=0;x<16;x++)if(pen(raw,16,x,y)){if(y<top)top=y;if(y+1>bottom)bottom=y+1;if(x<8)left=1;else right=1;}
 if(!bottom)CHECK(submitted_count==3);
 else{
  CHECK(submitted_count==4);const vdp1_cmdt_t*h=&submitted[2];unsigned width=left&&right?16:8;
  CHECK((((h->cmd_size>>8)&63)*8)==width);CHECK((h->cmd_size&255)==bottom-top);
  size_t base=WF_SATURN_VDP1_DYNAMIC_OFFSET+c->stream_tile*128u;
  CHECK(h->cmd_srca*8u==(width==8?base:base+top*8u));
  CHECK(h->cmd_colr==WF_SATURN_CRAM_SPRITE_FIRST+c->cram_slot*16u);
 }
 ++cases;
}
static void raw_case(const uint8_t*raw,unsigned flip,int x,int y){
 wf_vdp1_sprite_cmd_t c={.x=(int16_t)x,.y=(int16_t)y,.flip_x=flip&1,.flip_y=(flip>>1)&1,.cram_slot=15};
 begin(1,1);CHECK(g_ops.upload_tile_16x16_4bpp(&backend,0,raw,128)==WF_OK);CHECK(g_ops.emit_sprite_16x16(&backend,&c)==WF_OK);finish();compare_one(raw,&c);
}
static void synthetic(void){
 uint8_t unaligned[129],*raw=unaligned+1;
 for(unsigned top=0;top<16;top++)for(unsigned bottom=top+1;bottom<=16;bottom++)for(unsigned side=0;side<3;side++){
  memset(raw,0,128);unsigned x0=side==1?8:0,x1=side==0?8:16;
  for(unsigned y=top;y<bottom;y++)for(unsigned x=x0;x<x1;x++)pixel(raw,x,y,(x+y)%15+1);
  init_backend();for(unsigned f=0;f<4;f++)raw_case(raw,f,13,11);
 }
 memset(raw,0,128);init_backend();for(unsigned f=0;f<4;f++)raw_case(raw,f,-3,-5);
 /* Every individual pixel, including penF, and both clipping edges. */
 for(unsigned y=0;y<16;y++)for(unsigned x=0;x<16;x++){
  memset(raw,0,128);pixel(raw,x,y,15);init_backend();for(unsigned f=0;f<4;f++)raw_case(raw,f,(f&1)?40:-7,(f&2)?40:-7);
 }
}
static void cache_tests(void){
 memset(source_tiles,0,sizeof source_tiles);for(unsigned y=3;y<14;y++)for(unsigned x=0;x<8;x++)pixel(source_tiles[0],x,y,(x+y)%15+1);
 for(unsigned y=1;y<16;y++)for(unsigned x=8;x<16;x++)pixel(source_tiles[1],x,y,(x+2*y)%15+1);
 memcpy(source_tiles[2],source_tiles[0],128);memcpy(source_tiles[3],source_tiles[0],128);pixel(source_tiles[3],15,0,15);
 wf_vdp1_sprite_cmd_t c={.x=7,.y=9};init_backend();
 begin(1,1);CHECK(wf_saturn_yaul_upload_rom_tile(&backend,0,0,reader)==WF_OK);CHECK(reader_calls==1);CHECK(g_ops.emit_sprite_16x16(&backend,&c)==WF_OK);finish();compare_one(source_tiles[0],&c);CHECK(dma_count==1);
 for(unsigned f=0;f<4;f++){
  c.flip_x=f&1;c.flip_y=f>>1;begin(1,1);CHECK(wf_saturn_yaul_upload_rom_tile(&backend,0,0,reader)==WF_OK);CHECK(reader_calls==1);CHECK(g_ops.emit_sprite_16x16(&backend,&c)==WF_OK);finish();CHECK(dma_count==0);compare_one(source_tiles[0],&c);
 }
 /* Identical pixels under a different immutable source ID reuse VRAM. */
 begin(1,1);CHECK(wf_saturn_yaul_upload_rom_tile(&backend,0,2,reader)==WF_OK);CHECK(reader_calls==2);CHECK(g_ops.emit_sprite_16x16(&backend,&c)==WF_OK);finish();CHECK(dma_count==0);compare_one(source_tiles[0],&c);
 /* Generic uploads invalidate ROM identity, but compare logical pixels. */
 raw_case(source_tiles[0],0,7,9);CHECK(dma_count==0);CHECK(!(backend.rom_identity[0]&1));
 begin(1,1);CHECK(wf_saturn_yaul_upload_rom_tile(&backend,0,2,reader)==WF_OK);CHECK(reader_calls==3);CHECK(g_ops.emit_sprite_16x16(&backend,&c)==WF_OK);finish();CHECK(dma_count==0);
 /* A raw tile equal to compacted staging bytes is NOT the same picture. */
 uint8_t compacted_as_raw[128];memcpy(compacted_as_raw,stage,128);raw_case(compacted_as_raw,0,7,9);CHECK(dma_count==1);
 raw_case(source_tiles[0],0,7,9);CHECK(dma_count==1);raw_case(source_tiles[0],0,7,9);CHECK(dma_count==0);
 raw_case(source_tiles[3],0,7,9);CHECK(dma_count==1);raw_case(source_tiles[0],0,7,9);CHECK(dma_count==1);
 /* Equal compacted bytes with a changed original top need new geometry. */
 uint8_t moved_up[128]={0};memcpy(moved_up+2u*8u,source_tiles[0]+3u*8u,11u*8u);
 raw_case(moved_up,3,7,9);CHECK(dma_count==1);raw_case(source_tiles[0],0,7,9);CHECK(dma_count==1);
 /* Abort invalidates changed residency; no old identity may skip reload. */
 uint8_t before[128];memcpy(before,test_vram+WF_SATURN_VDP1_DYNAMIC_OFFSET,128);
 begin(1,1);CHECK(wf_saturn_yaul_upload_rom_tile(&backend,0,1,reader)==WF_OK);unsigned reads=reader_calls;CHECK(!(backend.resident[0]&1));g_ops.abort_frame(&backend);CHECK(!dma_count&&!render_calls);CHECK(!memcmp(before,test_vram+WF_SATURN_VDP1_DYNAMIC_OFFSET,128));
 begin(1,1);CHECK(wf_saturn_yaul_upload_rom_tile(&backend,0,1,reader)==WF_OK);CHECK(reader_calls==reads+1);CHECK(g_ops.emit_sprite_16x16(&backend,&c)==WF_OK);finish();CHECK(dma_count==1);compare_one(source_tiles[1],&c);
 /* An incomplete frame also leaves no committed list or valid new slot. */
 begin(1,1);CHECK(wf_saturn_yaul_upload_rom_tile(&backend,0,0,reader)==WF_OK);CHECK(g_ops.end_frame(&backend)==WF_BAD_STATE);CHECK(!render_calls&&!dma_count);
 begin(1,1);CHECK(wf_saturn_yaul_upload_rom_tile(&backend,0,0,reader)==WF_OK);CHECK(g_ops.emit_sprite_16x16(&backend,&c)==WF_OK);finish();CHECK(dma_count==1);compare_one(source_tiles[0],&c);
 /* Fixed 128-byte slot addresses and adjacent/split DMA runs are preserved. */
 init_backend();begin(3,3);for(unsigned i=0;i<3;i++)CHECK(wf_saturn_yaul_upload_rom_tile(&backend,i,i,reader)==WF_OK);
 memset(expected,0x80,sizeof expected);memset(observed,0x80,sizeof observed);
 for(unsigned i=0;i<3;i++){c.stream_tile=i;c.x=(int16_t)(i*13);CHECK(g_ops.emit_sprite_16x16(&backend,&c)==WF_OK);ref_draw(source_tiles[i],&c);}finish();CHECK(dma_count==1&&dma_sizes[0]==384&&dma_offsets[0]==WF_SATURN_VDP1_DYNAMIC_OFFSET);hw_draw();CHECK(!memcmp(expected,observed,sizeof expected));++cases;
 begin(3,0);CHECK(wf_saturn_yaul_upload_rom_tile(&backend,0,3,reader)==WF_OK);CHECK(wf_saturn_yaul_upload_rom_tile(&backend,1,1,reader)==WF_OK);CHECK(wf_saturn_yaul_upload_rom_tile(&backend,2,1,reader)==WF_OK);finish();CHECK(dma_count==2&&dma_sizes[0]==128&&dma_sizes[1]==128&&dma_offsets[1]==WF_SATURN_VDP1_DYNAMIC_OFFSET+256);
 /* Upper staging slot and rejected out-of-range accesses. */
 init_backend();begin(256,1);for(unsigned i=0;i<256;i++)CHECK(wf_saturn_yaul_upload_rom_tile(&backend,i,1,reader)==WF_OK);c.stream_tile=255;c.x=11;c.y=12;CHECK(g_ops.emit_sprite_16x16(&backend,&c)==WF_OK);finish();compare_one(source_tiles[1],&c);CHECK(dma_count==1&&dma_sizes[0]==32768);
 CHECK(g_ops.begin_frame(&backend,257,0,0)==WF_BLOCKED);begin(1,1);c.stream_tile=1;CHECK(g_ops.emit_sprite_16x16(&backend,&c)==WF_BAD_STATE);g_ops.abort_frame(&backend);
 /* Unstaged slots use uncropped fallback; failed commit submits nothing. */
 init_backend();begin(1,1);c.stream_tile=0;CHECK(g_ops.emit_sprite_16x16(&backend,&c)==WF_OK);CHECK(g_cmdts[2].cmd_size==0x0210);CHECK(g_ops.end_frame(&backend)==WF_BAD_STATE);CHECK(!render_calls&&!dma_count);
 init_backend();backend.defer_sync=0;raw_case(source_tiles[0],3,11,12);CHECK(sync_calls==1&&sync_waits==1);
}
static void atlas(const char*path){
 FILE*f=fopen(path,"rb");CHECK(f);uint8_t raw[128];unsigned n=0;
 while(fread(raw,1,128,f)==128){init_backend();for(unsigned flip=0;flip<4;flip++)raw_case(raw,flip,13,11);++n;}
 CHECK(feof(f));CHECK(n==EXPECTED_ATLAS_TILES);fclose(f);
}
int main(int argc,char**argv){
 CHECK(argc==2);CHECK(wf_saturn_memory_contract_validate()==WF_OK);synthetic();cache_tests();atlas(argv[1]);
 printf("{\"raster_cases\":%u,\"submitted_frames\":%u,\"atlas_tiles\":%u,\"atlas_flip_cases\":%u,\"backend_bytes\":%zu,\"status\":\"pass\"}\n",cases,frames,EXPECTED_ATLAS_TILES,4u*EXPECTED_ATLAS_TILES,sizeof backend);return 0;
}
