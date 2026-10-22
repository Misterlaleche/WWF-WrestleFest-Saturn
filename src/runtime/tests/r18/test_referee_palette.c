/* Regression: $10718 must allocate the referee palette before D1FC resolves
 * resource 12. Expected RAM/palette bytes come from unmodified 68000 ROM. */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <yaul.h>
#include "wf_r18_bootstrap.h"
#include "wf_r18_present.h"
#include "wf_r18_video.h"
#include "wf_r18_metasprite.h"

unsigned mock_sync_calls;
uint8_t mock_vram[0x80000];
uint32_t mock_cram[1024];
static uint8_t rom[0x80000], cache[0x80000], tiles[128*128];
static unsigned submitted;
void scu_dma_transfer(unsigned channel,void *dst,const void *src,size_t n)
{ assert(channel==0); memcpy(dst,src,n); }
void scu_dma_transfer_wait(unsigned channel) { assert(channel==0); }
void vdp1_sync_cmdt_list_put(vdp1_cmdt_list_t *list,unsigned offset)
{ assert(offset==0 && list->count>3); ++submitted; }
static wf_status_t read_rom(void *user,uint32_t at,void *dst,size_t n)
{ (void)user; if(at>sizeof rom || n>sizeof rom-at)return WF_BLOCKED; memcpy(dst,rom+at,n); return WF_OK; }
static int read_file(const char *name,uint32_t at,void *dst,size_t n)
{
 if(!strcmp(name,"ROM68K.BIN"))return read_rom(NULL,at,dst,n)==WF_OK?0:-1;
 /* Pixel indices do not affect palette upload or resource-bank selection. */
 if(!strcmp(name,"SPRITES.PAK")){memset(dst,0x11,n);return 0;}
 return -1;
}
static void load(const char *path,void *dst,size_t n)
{ FILE *f=fopen(path,"rb"); assert(f); assert(fread(dst,1,n,f)==n); assert(fgetc(f)==EOF); fclose(f); }
int main(int argc,char **argv)
{
 static wf_runtime_t rt;
 static wf_r18_video_t video;
 static wf_r18_sprite_record_t records[512];
 static const uint16_t roster[2][4]={{0,1,6,7},{3,2,10,11}};
 uint8_t expected[256+16+0x800];
 wf_r18_bootstrap_context_t boot={0};
 assert(argc==4); unsigned scenario=(unsigned)atoi(argv[3]); assert(scenario<2);
 load(argv[1],rom,sizeof rom); load(argv[2],expected,sizeof expected);
 video.read_rom=read_rom; boot.palette_2aea=wf_r18_palette_2aea; boot.user=&video;
 rt.m68k_regs.valid=1; rt.g.rng_state_valid=1; rt.g.rng_state=0x13579bdf;
 for(unsigned i=0;i<4;i++){
  wf_actor_raw_t *a=&rt.actors[i];
  wf_set_be16(a,0,0x8000);wf_set_be16(a,0x56,roster[scenario][i]);wf_set_u8(a,0xa1,1);
  rt.m68k_regs.d[i]=0xaabb0000u+i;
 }
 assert(wf_r18_tag_setup_1086c(&rt,&boot)==WF_OK);
 if(!(video.palette_map[12]&128)){
  fprintf(stderr,"FAIL: referee resource 12 is unallocated; D1FC selects wrestler bank %u\n",video.palette_map[12]&15);
  return 1;
 }
 for(unsigned i=0;i<4;i++)assert(rt.m68k_regs.d[i]==0xaabb0000u+i);
 /* $2988 clears 48 resource-map bytes after the 16 allocation bytes.
  * Later RAM ($1C1697, for example) belongs to globals, not palette IDs. */
 assert(memcmp(video.palette_map,expected,48)==0);
 assert(memcmp(video.palette_used,expected+256,16)==0);
 assert(memcmp(video.palettes+0x2000,expected+272,0x800)==0);
 wf_r18_metasprite_ctx_t parser={0};parser.read_rom=read_rom;parser.palette_map=video.palette_map;
 parser.records=records;parser.capacity=512;
 assert(wf_r18_metasprite_emit(&parser,12,0,100,100,0xffff)==WF_OK && parser.count>0);
 unsigned bank=video.palette_map[12]&15;
 for(size_t i=0;i<parser.count;i++)assert((records[i].bytes[9]&15)==bank);
 wf_r18_present_init(read_file,cache,sizeof cache,tiles,sizeof tiles);
 assert(wf_r18_rom_preload()==WF_OK);
 wf_set_be16(&rt.referee,4,0);wf_set_be16(&rt.referee,0x14,100);wf_set_be16(&rt.referee,0x16,100);
 wf_actor_raw_t *actors[]={&rt.referee};
 assert(wf_r18_present_batch(&rt,actors,1,&video)==WF_OK && submitted==1);
 for(unsigned i=0;i<16;i++){
  unsigned word=i?((unsigned)rom[0x30a2+i*2]<<8)|rom[0x30a3+i*2]:0;
  uint32_t rgb=((word&15)*17u)|(((word>>4)&15)*17u<<8)|(((word>>8)&15)*17u<<16);
  assert(mock_cram[WF_SATURN_CRAM_SPRITE_FIRST+bank*16+i]==rgb);
 }
 printf("PASS referee ROM palette 12 -> bank %u -> %zu sprite records -> Saturn CRAM; all five allocations match ROM\n",bank,parser.count);
 return 0;
}
