/* Exercise the production renderer's pack loader/cache against every original
 * tile byte, including cache eviction, repeated hits and the cold fallback. */
#include "wf_r18_present.h"
#include "wf_saturn_memory.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static const char *pack_path,*sprite_path;
static unsigned cold_reads;
static uint8_t cache[WF_SATURN_LWRAM_TILE_BYTES],rom[1024];
extern volatile uint32_t wp1980_sprite_block_decodes,wp1980_sprite_block_hits;
static int read_file(const char *name,uint32_t offset,void *out,size_t bytes){
 const char *path;
 if(!strcmp(name,"HOTSPR.BIN"))path=pack_path;
 else if(!strcmp(name,"SPRITES.PAK")){path=sprite_path;++cold_reads;}
 else return -1;
 FILE *f=fopen(path,"rb");if(!f)return -1;
 int error=fseek(f,(long)offset,SEEK_SET)||fread(out,1,bytes,f)!=bytes;
 fclose(f);return error?-1:0;
}
static unsigned be16(const uint8_t *p){return (unsigned)p[0]<<8|p[1];}
int main(int argc,char **argv){
 assert(argc==3);pack_path=argv[1];sprite_path=argv[2];
 FILE *f=fopen(pack_path,"rb");assert(f);
 uint8_t header[32];assert(fread(header,1,32,f)==32);assert(!memcmp(header,"R18F",4)||!memcmp(header,"R18L",4)||!memcmp(header,"R18Z",4)||!memcmp(header,"R18U",4));
 unsigned count=be16(header+4);uint8_t *ids=malloc(count*2u);assert(ids);
 assert(fread(ids,2,count,f)==count);fclose(f);
 uint8_t *source=malloc(8388608);assert(source);f=fopen(sprite_path,"rb");assert(f);
 assert(fread(source,1,8388608,f)==8388608);fclose(f);
 wf_r18_present_init(read_file,rom,sizeof rom,cache,sizeof cache);
 uint8_t *expanded=NULL;
 if(!memcmp(header,"R18U",4)){
  expanded=malloc(count*128u+32u);assert(expanded);memset(expanded,0xa5,count*128u+32u);
  assert(wf_r18_hot_sprites_load_raw(expanded,count*128u-1u)==WF_BLOCKED);
  assert(wf_r18_hot_sprites_load_raw(expanded,count*128u)==WF_OK);
  for(unsigned i=0;i<32u;i++)assert(expanded[count*128u+i]==0xa5u);
 }else assert(wf_r18_hot_sprites_load()==WF_OK);
 unsigned checks=0;uint32_t random=1980;
 for(unsigned pass=0;pass<3;pass++)for(unsigned i=0;i<count;i++){
  random=random*1664525u+1013904223u;
  unsigned j=pass==0?i:pass==1?count-i-1u:random%count;
  unsigned tile=be16(ids+j*2u);const uint8_t *pixels;
  assert(wf_r18_tile_read((uint16_t)tile,&pixels)==WF_OK);
  assert(!memcmp(pixels,source+tile*128u,128));++checks;
 }
 assert(cold_reads==0);
 unsigned cold=0;
 for(;;cold++){unsigned found=0;for(unsigned j=0;j<count;j++)if(be16(ids+j*2u)==cold){found=1;break;}if(!found)break;}
 const uint8_t *pixels;
 assert(wf_r18_tile_read((uint16_t)cold,&pixels)==WF_OK);
 assert(!memcmp(pixels,source+cold*128u,128));assert(cold_reads==1);
 assert(wf_r18_tile_read((uint16_t)cold,&pixels)==WF_OK);assert(cold_reads==1);
 printf("PASS: %u byte-exact tile checks, %u original IDs, block_decodes=%u block_hits=%u; hot CD reads0, cold fallback verified\n",checks,count,(unsigned)wp1980_sprite_block_decodes,(unsigned)wp1980_sprite_block_hits);
 free(expanded);free(source);free(ids);return 0;
}
