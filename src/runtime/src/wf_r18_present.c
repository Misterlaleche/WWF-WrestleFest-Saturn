#include "wf_r18_present.h"
#include "wf_renderer_contract.h"
#include <string.h>
static wf_r18_file_read_fn read_file;
static uint8_t *rom_data,*tile_data;static size_t rom_slots,tile_slots,rom_capacity;
static int rom_preloaded;
static uint16_t rom_page[256],rom_slot[512];static uint32_t rom_age[256],rom_clock;
static uint16_t tile_tags[6144];static uint8_t tile_valid[6144];
/* Sector-aligned header stores sorted tile IDs; pixel payload resides in the
 * first tile-cache slots. The remaining slots retain exact on-demand fallback. */
static uint8_t hot_header[8192];
static size_t hot_count;
static wf_r18_sprite_record_t records[512];static wf_vdp1_frame_plan_t plan;
static wf_actor_raw_t *composed[WF_ACTOR_SLOTS];static size_t composed_count;
static uint8_t dma_stage[WF_VDP1_STREAM_TILES_MAX*WF_VDP1_TILE_BYTES_16X16_4BPP] __attribute__((aligned(32)));
static wf_saturn_yaul_backend_t backend;
static unsigned busy;
void wf_r18_present_init(wf_r18_file_read_fn fn,void *rc,size_t rn,void *tc,size_t tn){read_file=fn;rom_data=rc;rom_capacity=rn;rom_preloaded=0;tile_data=tc;rom_slots=rn/1024;tile_slots=tn/128;if(rom_slots>256)rom_slots=256;if(tile_slots>6144)tile_slots=6144;hot_count=0;for(unsigned i=0;i<256;i++)rom_page[i]=0xffffu;for(unsigned i=0;i<512;i++)rom_slot[i]=0xffffu;memset(rom_age,0,sizeof rom_age);rom_clock=0;memset(tile_valid,0,sizeof tile_valid);wf_saturn_yaul_backend_init(&backend,dma_stage,sizeof dma_stage);backend.defer_sync=1;}
wf_status_t wf_r18_rom_preload(void){
 if(!read_file||!rom_data||rom_capacity<524288u)return WF_BLOCKED;
 if(read_file("ROM68K.BIN",0,rom_data,524288u))return WF_BLOCKED;
 rom_preloaded=1;return WF_OK;
}
wf_status_t wf_r18_rom_pages_preload(const uint16_t *pages,size_t count){
 if(!read_file||!rom_data||!pages||!count||count>rom_slots)return WF_BLOCKED;
 for(size_t i=0;i<count;i++)if(pages[i]>=512u||(i&&pages[i-1]>=pages[i]))return WF_BLOCKED;
 rom_preloaded=0;
 for(unsigned i=0;i<256u;i++)rom_page[i]=0xffffu;
 for(unsigned i=0;i<512u;i++)rom_slot[i]=0xffffu;
 memset(rom_age,0,sizeof rom_age);rom_clock=0;
 for(size_t first=0;first<count;){
  size_t end=first+1u;while(end<count&&pages[end]==pages[end-1u]+1u)end++;
  if(read_file("ROM68K.BIN",(uint32_t)pages[first]*1024u,rom_data+first*1024u,(end-first)*1024u))return WF_BLOCKED;
  for(size_t i=first;i<end;i++){rom_page[i]=pages[i];rom_slot[pages[i]]=(uint16_t)i;rom_age[i]=++rom_clock;}
  first=end;
 }
 return WF_OK;
}
wf_status_t wf_r18_rom_read(void*u,uint32_t a,void*dest,size_t n){uint8_t*out=dest;(void)u;if(!read_file||!rom_slots||a>0x80000u||n>0x80000u-a)return WF_BLOCKED;
 if(rom_preloaded){memcpy(out,rom_data+a,n);return WF_OK;}
 while(n){uint32_t page=a/1024;size_t slot=rom_slot[page],off=a%1024,take=1024-off;if(take>n)take=n;
 if(slot==0xffffu){
  slot=0;
  for(size_t i=0;i<rom_slots;i++)
   if(rom_page[i]==0xffffu){slot=i;break;}
   else if(rom_age[i]<rom_age[slot])slot=i;
  if(rom_page[slot]!=0xffffu)rom_slot[rom_page[slot]]=0xffffu;
  rom_page[slot]=0xffffu;
  if(read_file("ROM68K.BIN",page*1024,rom_data+slot*1024,1024))return WF_BLOCKED;
  rom_page[slot]=(uint16_t)page;rom_slot[page]=(uint16_t)slot;
 }
 rom_age[slot]=++rom_clock;
 memcpy(out,rom_data+slot*1024+off,take);out+=take;a+=(uint32_t)take;n-=take;}
 return WF_OK;}
static uint16_t hot_id(size_t index){
 const uint8_t *p=hot_header+8u+index*2u;
 return (uint16_t)((p[0]<<8)|p[1]);
}
wf_status_t wf_r18_hot_sprites_load(void){
 size_t count,header_bytes;
 hot_count=0;
 memset(tile_valid,0,sizeof tile_valid);
 if(!read_file||!tile_data||tile_slots<2u||
    read_file("HOTSPR.BIN",0,hot_header,4096u))return WF_BLOCKED;
 if(memcmp(hot_header,"R18H",4)||hot_header[6]!=0u||hot_header[7]!=128u)
  return WF_BLOCKED;
 count=(size_t)((hot_header[4]<<8)|hot_header[5]);
 if(!count||count>(sizeof hot_header-8u)/2u||count>=tile_slots)
  return WF_BLOCKED;
 /* Keep existing packs byte-compatible. Larger ID tables occupy four CD
  * sectors instead of two; the count unambiguously selects the payload. */
 header_bytes=count>2044u?8192u:4096u;
 if(header_bytes>4096u&&read_file("HOTSPR.BIN",4096u,hot_header+4096u,4096u))
  return WF_BLOCKED;
 for(size_t i=1;i<count;i++)if(hot_id(i)<=hot_id(i-1u))
  return WF_BLOCKED;
 if(read_file("HOTSPR.BIN",(uint32_t)header_bytes,tile_data,count*128u))
  return WF_BLOCKED;
 hot_count=count;
 return WF_OK;
}
size_t wf_r18_hot_sprites_bytes(void){
 return hot_count?(hot_count>2044u?8192u:4096u)+hot_count*128u:0u;
}
wf_status_t wf_r18_tile_read(uint16_t tile,const uint8_t **out){
 size_t lo=0,hi=hot_count,slot,cold;
 if(!out||!tile_data||!read_file||!tile_slots)return WF_BLOCKED;
 while(lo<hi){size_t mid=lo+(hi-lo)/2u;
  if(hot_id(mid)<tile)lo=mid+1u;else hi=mid;}
 if(lo<hot_count&&hot_id(lo)==tile){*out=tile_data+lo*128u;return WF_OK;}
 cold=tile_slots-hot_count;
 if(!cold)return WF_BLOCKED;
 slot=hot_count+(size_t)tile%cold;
 if(!tile_valid[slot]||tile_tags[slot]!=tile){
  if(read_file("SPRITES.PAK",(uint32_t)tile*128u,tile_data+slot*128u,128u))
   return WF_BLOCKED;
  tile_tags[slot]=tile;tile_valid[slot]=1u;
 }
 *out=tile_data+slot*128u;
 return WF_OK;
}
static uint16_t be16(const uint8_t*p){return (uint16_t)((p[0]<<8)|p[1]);}
static wf_actor_raw_t *actor_link(wf_runtime_t*r,uint32_t address){return wf_actor_from_rom_ptr(r,address,NULL);}
static int add_cmd(uint16_t tile,int x,int y,unsigned fx,unsigned fy,unsigned palette){unsigned slot;
 if(x>=320||x+16<=0||y>=240||y+16<=0)return 1;
 if(plan.cmd_count==WF_VDP1_COMMANDS_MAX)return 0;
 for(slot=0;slot<plan.tile_count;slot++)if(plan.tile_upload[slot].source_tile==tile)break;
 if(slot==plan.tile_count){if(slot==WF_VDP1_STREAM_TILES_MAX)return 0;plan.tile_upload[slot].source_tile=tile;plan.tile_upload[slot].stream_slot=(uint16_t)slot;plan.tile_count++;}
 wf_vdp1_sprite_cmd_t*c=&plan.cmd[plan.cmd_count++];memset(c,0,sizeof *c);c->x=(int16_t)x;c->y=(int16_t)y;c->stream_tile=(uint16_t)slot;c->cram_slot=(uint8_t)palette;c->flip_x=(uint8_t)fx;c->flip_y=(uint8_t)fy;return 1;}
wf_status_t wf_r18_present_batch(wf_runtime_t*r,wf_actor_raw_t*const*actors,size_t n,void*u){wf_r18_video_t*v=u;wf_r18_metasprite_ctx_t parser;wf_status_t status=WF_OK;const wf_saturn_render_ops_t*ops=wf_saturn_yaul_render_ops();
 if(!r||!v||(!actors&&n)||busy)return WF_BAD_STATE;busy=1;memset(&parser,0,sizeof parser);parser.read_rom=wf_r18_rom_read;parser.palette_map=v->palette_map;parser.records=records;parser.capacity=512;parser.d6=r->m68k_regs.d[6];parser.d7=r->m68k_regs.d[7];
 composed_count=0;
 for(size_t i=0;i<n;i++){wf_actor_raw_t*a=actors[i];wf_actor_raw_t*partner;if(wf_u8(a,0x21)==255||(wf_be16(a,4)&0x7fff)==0x7fff)continue;partner=actor_link(r,wf_be32(a,0x26));status=wf_r18_metasprite_emit(&parser,wf_be16(a,2),wf_be16(a,4),wf_be16(a,0x14),wf_be16(a,0x16),partner?wf_be16(partner,2):0xffff);if(status!=WF_OK)goto done;if(composed_count<WF_ACTOR_SLOTS)composed[composed_count++]=a;}
 memset(&plan,0,sizeof plan);
 for(size_t i=0;i<parser.count;i++){const uint8_t*b=records[i].bytes;unsigned attr=be16(b+2);if(!(attr&1))continue;unsigned chain=(attr>>5)&7,fx=(attr>>4)&1,fy=(attr>>3)&1,palette=b[9]&15;unsigned tile=wf_sprite_number_align((unsigned)b[5]|((unsigned)b[7]<<8),chain);int x=wf_sprite_x_decode((unsigned)b[11]|((attr&4)<<6)),y=wf_sprite_y_decode((unsigned)b[1]|((attr&2)<<7));
 for(unsigned j=0;j<=chain;j++){int yy=fy?y-(int)chain*16+(int)j*16:y-(int)j*16;if(!add_cmd((uint16_t)(tile+j),x,yy,fx,fy,palette)){status=WF_BLOCKED;goto done;}}}
 status=ops->begin_frame(&backend,plan.tile_count,16,plan.cmd_count);if(status!=WF_OK)goto done;
 for(unsigned i=0;i<plan.tile_count;i++){const uint8_t*p;status=wf_r18_tile_read(plan.tile_upload[i].source_tile,&p);if(status==WF_OK)status=ops->upload_tile_16x16_4bpp(&backend,(uint16_t)i,p,128);if(status!=WF_OK)goto abort;}
 for(unsigned i=0;i<16;i++){status=ops->upload_palette_xbgr444_16(&backend,(uint8_t)i,v->palettes+0x2000+i*128,32);if(status!=WF_OK)goto abort;}
 for(unsigned i=0;i<plan.cmd_count;i++){status=ops->emit_sprite_16x16(&backend,&plan.cmd[i]);if(status!=WF_OK)goto abort;}
 status=ops->end_frame(&backend);if(status!=WF_OK)goto done;
 /* $293E consumes a resolved link after rendering its composition. */
 for(size_t i=0;i<composed_count;i++)if((wf_u8(composed[i],0x26)&128)&&!wf_be16(composed[i],0xfe))wf_set_be32(composed[i],0x26,0);
 r->m68k_regs.d[6]=parser.d6;r->m68k_regs.d[7]=parser.d7;goto done;
abort:ops->abort_frame(&backend);
done:busy=0;return status;}
unsigned wf_r18_present_commands(void){return plan.cmd_count;}
