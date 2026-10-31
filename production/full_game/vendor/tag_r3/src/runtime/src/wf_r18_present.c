#include "wf_saturn_yaul_backend.h"
#include "wf_r18_present.h"
#include "wf_renderer_contract.h"
#include <string.h>
#ifdef WF_PLATFORM_SATURN
#include "wp2015_secondary.h"
#endif
#ifdef WF_R18_PROFILE_START
volatile uint32_t wf_r18_rom_line_hits, wf_r18_rom_line_misses;
#endif
static wf_r18_file_read_fn read_file;
static uint8_t *rom_data,*tile_data;static size_t rom_slots,tile_slots,rom_capacity;
static int rom_preloaded;
#ifdef WF_PLATFORM_SATURN
/* Valid only while the HOTSPR-owned prefix of tile_tags remains unused. */
static unsigned hot_tag_rom_ready;
#endif
static void hot_tag_rom_reset(void){
#ifdef WF_PLATFORM_SATURN
 hot_tag_rom_ready=0;
#endif
}
static uint16_t rom_page[256],rom_slot[512];static uint32_t rom_age[256],rom_clock;
/* Fourteen Royal pages stay pinned despite cold misses in the LWRAM cache. */
#define WF_R18_ROM_EXTRA_CAPACITY 14u
static uint8_t rom_extra[WF_R18_ROM_EXTRA_CAPACITY][1024] __attribute__((aligned(32)));
static uint8_t rom_extra_slot[512];
/* Immutable metadata is repeatedly read as bytes/words/longs. Keep small
 * lines in HWRAM instead of revisiting the slow LWRAM page for every field.
 * Lines never cross a1KB source page, so a fill cannot add a CD page read. */
#define WF_R18_ROM_LINE_COUNT 64u
static uint8_t rom_words[WF_R18_ROM_LINE_COUNT][32] __attribute__((aligned(32)));
static uint32_t rom_word_tag[WF_R18_ROM_LINE_COUNT];
static wf_r18_metasprite_cache_t metasprite_cache;
static void rom_caches_reset(void){memset(rom_word_tag,255,sizeof rom_word_tag);wf_r18_metasprite_cache_reset(&metasprite_cache);}
static void rom_extra_reset(void){hot_tag_rom_reset();memset(rom_extra_slot,255,sizeof rom_extra_slot);}
/* Saturn cannot own more slots than the declared complete LWRAM tile lease.
 * Host callers keep the existing larger general-purpose capacity. */
#ifdef WF_PLATFORM_SATURN
#define WF_R18_TILE_METADATA_SLOTS (WF_SATURN_LWRAM_TILE_BYTES/128u)
_Static_assert(WF_SATURN_LWRAM_FRONT_TILE_BYTES<=WF_SATURN_LWRAM_TILE_BYTES,
               "Frontend tile lease must fit native metadata");
#else
#define WF_R18_TILE_METADATA_SLOTS 6144u
#endif
static uint16_t tile_tags[WF_R18_TILE_METADATA_SLOTS];
static uint8_t tile_valid[WF_R18_TILE_METADATA_SLOTS];
static uint16_t cold_hand;
/* Sector-aligned R18H IDs start at8, followed by raw128-byte tiles.
 * R18D IDs start at24; BE32 fields8/12/16/20 hold chunk count, mapping bytes,
 * dictionary bytes and header bytes. The file holds four/eight/32 BE16 indices
 * per tile, padding to 32 bytes, then exact 32/16/4-byte chunks. The first
 * bounded index prefix stays in HWRAM; any remainder precedes the dictionary
 * in LWRAM tile_data. Cold slots follow the resident payload's next
 * 128-byte boundary. The scratch tile is consumed synchronously by upload. */
/* The native fixture needs the fixed header plus 6474 IDs. The rest of
 * its 16 KiB file header is padding and need not occupy HWRAM. Host tools
 * keep their original general-purpose capacity. Payload offsets continue
 * to use the full sector-aligned header_bytes from the file format. */
#ifdef WF_PLATFORM_SATURN
/* After all file/header checks, the same storage becomes an exact rank
 * index for the 16-bit tile-ID universe: 8 KiB membership + 4 KiB prefixes.
 * No extra runtime allocation; source order and dictionary indices stay exact. */
static union {
 uint8_t bytes[24u+WF_SATURN_VDP1_CROP_MAX_TILES*2u];
 struct {uint32_t bits[2048];uint16_t prefix[2048];} rank;
} hot_lookup;
#define hot_header hot_lookup.bytes
static unsigned hot_rank_ready;
_Static_assert(sizeof hot_lookup.rank<=sizeof hot_header,"Rank index fits discarded ID header");
#else
static uint8_t hot_header[16384];
#endif
static size_t hot_count,hot_slots,hot_header_bytes,hot_payload_bytes,hot_dictionary_offset,hot_map_spill;
static unsigned hot_id_offset;
static unsigned hot_chunk_bytes;
static unsigned hot_parts;
static unsigned hot_crop_ready;
static uint16_t hot_crop_source,hot_crop_index;
static unsigned hot_crop_valid;
#ifdef WF_PLATFORM_SATURN
#ifndef WF_R18_CROP_BASE
#define WF_R18_CROP_BASE ((volatile uint16_t *)(uintptr_t)(0x25c00000u+WF_SATURN_VDP1_CROP_OFFSET))
#endif
_Static_assert(WF_SATURN_VDP1_CROP_OFFSET>=0x42000u,"Crop table follows both palette queues");
_Static_assert(WF_SATURN_VDP1_CROP_END<=WF_SATURN_VDP1_DYNAMIC_OFFSET,"Crop table cannot touch either texture bank");
#endif
static uint8_t hot_tile[128] __attribute__((aligned(32)));
/* Bounded immutable index table; HWRAM keeps frequently used TAG tile maps
 * out of the LWRAM dictionary lease. */
static uint8_t hot_indices[WF_SATURN_HWRAM_HOT_INDEX_BYTES] __attribute__((aligned(32)));
static wf_r18_sprite_record_t records[512];static wf_vdp1_frame_plan_t plan;
static wf_actor_raw_t *composed[WF_ACTOR_SLOTS];static size_t composed_count;
static uint8_t dma_stage[WF_VDP1_STREAM_TILES_MAX*WF_VDP1_TILE_BYTES_16X16_4BPP] __attribute__((aligned(32)));
static wf_saturn_yaul_backend_t backend;
static unsigned busy;
static unsigned backup_workspace_leased;
static size_t last_record_count;
static size_t secondary_lease_slots;
static unsigned secondary_lease_open;
/* Invalidate publication before restoring or replacing the borrowed tail.
 * The secondary reset neither accesses the lease nor performs file I/O. */
static void secondary_lease_reset(void){
#ifdef WF_PLATFORM_SATURN
 wp2015_secondary_reset();
#endif
 tile_slots+=secondary_lease_slots;secondary_lease_slots=0u;
 secondary_lease_open=1u;
}
wf_status_t wf_r18_present_secondary_lease(size_t bytes,void **base){
 if(!base||!bytes||(bytes&127u)||!tile_data||!hot_count||
    !secondary_lease_open||secondary_lease_slots||busy||backend.active||backup_workspace_leased||
    hot_slots>=tile_slots)return WF_BLOCKED;
 size_t slots=bytes/128u;
 /* Strict comparison retains at least one complete original cold slot.
  * No multiplication occurs until the requested count is proven bounded. */
 if(slots>=tile_slots-hot_slots)return WF_BLOCKED;
 tile_slots-=slots;secondary_lease_slots=slots;cold_hand=0;
 *base=tile_data+tile_slots*128u;
 return WF_OK;
}
void *wf_r18_present_backup_workspace_begin(size_t bytes){
 if(backup_workspace_leased||busy||backend.active||bytes>sizeof dma_stage)return NULL;
 backup_workspace_leased=1;
 /* BUP replaces these bytes. No stale VDP1 texture may be treated as
  * resident when frontend drawing resumes after the synchronous call. */
 memset(backend.resident,0,sizeof backend.resident);
 memset(backend.rom_identity,0,sizeof backend.rom_identity);
 return dma_stage;
}
void wf_r18_present_backup_workspace_end(void){backup_workspace_leased=0;}
/* Half-full at the maximum256 unique tiles. The index changes lookup cost,
 * not first-use order: slots remain exactly those of the arcade draw stream. */
static uint16_t plan_tile_index[512];
/* 9 bits hold the 1-based stream slot; the upper 7 identify this frame.
 * Epoch wrap clears 1 KiB once per 127 frames instead of every frame. */
static uint8_t plan_tile_epoch;
void wf_r18_present_init(wf_r18_file_read_fn fn,void *rc,size_t rn,void *tc,size_t tn){secondary_lease_reset();read_file=fn;rom_data=rc;rom_capacity=rn;rom_preloaded=0;tile_data=tc;rom_slots=rn/1024;tile_slots=tn/128;if(rom_slots>256)rom_slots=256;if(tile_slots>WF_R18_TILE_METADATA_SLOTS)tile_slots=WF_R18_TILE_METADATA_SLOTS;hot_count=hot_slots=hot_header_bytes=hot_payload_bytes=hot_dictionary_offset=hot_map_spill=0;hot_id_offset=8;hot_chunk_bytes=hot_parts=0;hot_crop_ready=hot_crop_valid=0;
#ifdef WF_PLATFORM_SATURN
hot_rank_ready=0;
#endif
cold_hand=0;for(unsigned i=0;i<256;i++){rom_page[i]=0xffffu;}for(unsigned i=0;i<512;i++){rom_slot[i]=0xffffu;}memset(rom_age,0,sizeof rom_age);rom_clock=0;last_record_count=0;rom_caches_reset();rom_extra_reset();memset(tile_valid,0,sizeof tile_valid);memset(plan_tile_index,0,sizeof plan_tile_index);plan_tile_epoch=0u;wf_saturn_yaul_backend_init(&backend,dma_stage,sizeof dma_stage);backend.defer_sync=1;}
wf_status_t wf_r18_rom_preload(void){
 rom_caches_reset();
 rom_extra_reset();
 if(!read_file||!rom_data||rom_capacity<524288u)return WF_BLOCKED;
 if(read_file("ROM68K.BIN",0,rom_data,524288u))return WF_BLOCKED;
 rom_preloaded=1;return WF_OK;
}
wf_status_t wf_r18_rom_pages_preload(const uint16_t *pages,size_t count){
 rom_caches_reset();
 if(!read_file||!rom_data||!pages||!count||count>rom_slots)return WF_BLOCKED;
 for(size_t i=0;i<count;i++)if(pages[i]>=512u||(i&&pages[i-1]>=pages[i]))return WF_BLOCKED;
 rom_preloaded=0;
 rom_extra_reset();
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
wf_status_t wf_r18_rom_extra_pages_preload(const uint16_t *pages,size_t count){
 if(!read_file||!pages||!count||count>WF_R18_ROM_EXTRA_CAPACITY||rom_preloaded)return WF_BLOCKED;
 for(size_t i=0;i<count;i++)
  if(pages[i]>=512u||(i&&pages[i-1]>=pages[i]))
   return WF_BLOCKED;
 rom_caches_reset();rom_extra_reset();
 for(size_t i=0;i<count;i++){
  if(rom_slot[pages[i]]!=0xffffu)
   memcpy(rom_extra[i],rom_data+(size_t)rom_slot[pages[i]]*1024u,1024u);
  else if(read_file("ROM68K.BIN",(uint32_t)pages[i]*1024u,rom_extra[i],1024u)){
   rom_extra_reset();return WF_BLOCKED;
  }
  rom_extra_slot[pages[i]]=(uint8_t)i;
 }
 return WF_OK;
}
static wf_status_t rom_read_bytes(uint32_t a,void*dest,size_t n){uint8_t*out=dest;if(!read_file||!rom_slots||a>0x80000u||n>0x80000u-a)return WF_BLOCKED;
 if(rom_preloaded){memcpy(out,rom_data+a,n);return WF_OK;}
 while(n){uint32_t page=a/1024;size_t slot=rom_slot[page],off=a%1024,take=1024-off;if(take>n)take=n;
 if(rom_extra_slot[page]!=0xffu){memcpy(out,rom_extra[rom_extra_slot[page]]+off,take);out+=take;a+=(uint32_t)take;n-=take;continue;}
#ifdef WF_PLATFORM_SATURN
 /* Preserve the existing fourteen HWRAM pages above. These seven immutable
  * animation pages use only tags belonging to already resident HOTSPR data;
  * cold tags still begin at hot_slots and can never enter this prefix. */
 if(hot_tag_rom_ready&&page>=166u&&page<173u){
  const uint8_t *source=(const uint8_t *)tile_tags+(page-166u)*1024u;
  memcpy(out,source+off,take);out+=take;a+=(uint32_t)take;n-=take;continue;
 }
#endif
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
wf_status_t wf_r18_rom_read(void*u,uint32_t a,void*dest,size_t n){
 (void)u;
 if(!read_file||!rom_slots||a>0x80000u||n>0x80000u-a)return WF_BLOCKED;
 if(n&&n<=4u&&(a&31u)+n<=32u){
  uint32_t line=a>>5;unsigned slot=line&(WF_R18_ROM_LINE_COUNT-1u);
  if(rom_word_tag[slot]!=line){
#ifdef WF_R18_PROFILE_START
   ++wf_r18_rom_line_misses;
#endif
   rom_word_tag[slot]=0xffffffffu;
   wf_status_t s=rom_read_bytes(a&~31u,rom_words[slot],32u);
   if(s!=WF_OK)return s;
   rom_word_tag[slot]=line;
  }
#ifdef WF_R18_PROFILE_START
  else ++wf_r18_rom_line_hits;
#endif
  const uint8_t *p=rom_words[slot]+(a&31u);uint8_t *out=dest;
  for(size_t i=0;i<n;i++)out[i]=p[i];
  return WF_OK;
 }
 return rom_read_bytes(a,dest,n);
}
static uint16_t hot_id(size_t index){
 const uint8_t *p=hot_header+hot_id_offset+index*2u;
 return (uint16_t)((p[0]<<8)|p[1]);
}
#ifdef WF_PLATFORM_SATURN
static uint32_t crop_be32(const uint8_t*p){return (uint32_t)p[0]<<24|(uint32_t)p[1]<<16|(uint32_t)p[2]<<8|p[3];}
static uint32_t crop_hash(uint32_t hash,const uint8_t*p,size_t bytes){
 while(bytes--){hash^=*p++;hash*=16777619u;}return hash;
}
/* CDFS first reads HWRAM scratch. Only aligned 16-bit CPU stores target
 * VDP1; this never requests an unsupported B-bus to B-bus SCU transfer.
 * As with HOTSPR itself, this is called after present_init and before a
 * frame starts: the existing DMA stage is idle and can hold this 13 KiB
 * startup read without an additional HWRAM allocation.
 * Missing/stale/truncated metadata simply keeps the original pixel scan. */
static void hot_crop_load(void){
 hot_crop_ready=hot_crop_valid=0;
 if(!hot_count||hot_count>WF_SATURN_VDP1_CROP_MAX_TILES||read_file("CROP.BIN",0,hot_tile,16u))return;
 if(memcmp(hot_tile,"R18C",4)||hot_tile[4]!=0u||hot_tile[5]!=1u||
    (size_t)((hot_tile[6]<<8)|hot_tile[7])!=hot_count)return;
 uint32_t ids=crop_hash(2166136261u,hot_header+hot_id_offset,hot_count*2u);
 if(crop_be32(hot_tile+8u)!=ids)return;
 uint32_t expected=crop_be32(hot_tile+12u);
 if(read_file("CROP.BIN",16u,dma_stage,hot_count*2u)||
    crop_hash(2166136261u,dma_stage,hot_count*2u)!=expected)return;
 for(size_t i=0;i<hot_count;i++){
  unsigned meta=dma_stage[i*2u],height=dma_stage[i*2u+1u],top=meta&31u;
  if((meta&128u)||top>16u||height>16u||top+height>16u||
     ((meta&64u)&&!(meta&32u))||(!height&&(meta!=16u))||
     (height&&top==16u))return;
  WF_R18_CROP_BASE[i]=(uint16_t)((meta<<8)|height);
 }
 hot_crop_ready=1;
}
#else
static void hot_crop_load(void){hot_crop_ready=hot_crop_valid=0;}
#endif
/* R18P uses the same bounded HWRAM index: aligned BE32 offset followed
 * by [crop:16][reserved:16]. Reads are native words on the big-endian SH-2. */
static uint32_t hot_packed_word(const uint8_t *p){
 uint32_t value;__builtin_memcpy(&value,__builtin_assume_aligned(p,4u),4u);
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
 value=__builtin_bswap32(value);
#endif
 return value;
}
/* Source may be an unaligned caller-owned lease. Destination always names
 * a four-byte-aligned row/half in hot_tile. Each word is copied as bytes,
 * retaining the portable fallback and strict-aliasing correctness. */
static void hot_packed_copy(uint8_t *dest,const uint8_t *source,unsigned words,unsigned stride){
 if(!((uintptr_t)source&3u)){
  while(words--){uint32_t value;
   __builtin_memcpy(&value,__builtin_assume_aligned(source,4u),4u);
   __builtin_memcpy(__builtin_assume_aligned(dest,4u),&value,4u);
   source+=4u;dest+=stride;
  }
 }else{
  while(words--){dest[0]=source[0];dest[1]=source[1];dest[2]=source[2];dest[3]=source[3];source+=4u;dest+=stride;}
 }
}
static uint16_t hot_crop_read(uint16_t source){
 if(hot_chunk_bytes==128u&&hot_crop_valid&&source==hot_crop_source)
  return (uint16_t)(hot_packed_word(hot_indices+(size_t)hot_crop_index*8u+4u)>>16);
#ifdef WF_PLATFORM_SATURN
 if(hot_crop_ready&&hot_crop_valid&&source==hot_crop_source)
  return WF_R18_CROP_BASE[hot_crop_index];
#else
 (void)source;
#endif
 return 0xffffu;
}
/* Rank is the number of set IDs preceding this ID, so its index is
 * exactly the former binary-search position. All 65536 IDs are representable. */
#ifdef WF_PLATFORM_SATURN
static unsigned hot_popcount(uint32_t v){
 v-=((v>>1)&0x55555555u);
 v=(v&0x33333333u)+((v>>2)&0x33333333u);
 v=(v+(v>>4))&0x0f0f0f0fu;
 v+=v>>8;v+=v>>16;return v&63u;
}
static void hot_rank_build(void){
 /* Startup-only scratch is no longer needed by the completed CROP load.
  * Finish reading every original ID before changing the header union. */
 memset(dma_stage,0,8192u);
 for(size_t i=0;i<hot_count;i++){
  unsigned id=hot_id(i);dma_stage[id>>3]|=(uint8_t)(1u<<(id&7u));
 }
 unsigned rank=0;
 for(unsigned i=0;i<2048u;i++){
  const uint8_t *p=dma_stage+i*4u;
  uint32_t bits=(uint32_t)p[0]|((uint32_t)p[1]<<8)|((uint32_t)p[2]<<16)|((uint32_t)p[3]<<24);
  hot_lookup.rank.bits[i]=bits;hot_lookup.rank.prefix[i]=(uint16_t)rank;
  rank+=hot_popcount(bits);
 }
 hot_rank_ready=1;
}
#else
static void hot_rank_build(void){}
#endif
static size_t hot_find(uint16_t tile){
#ifdef WF_PLATFORM_SATURN
 if(hot_rank_ready){
  unsigned index=tile>>5;uint32_t bit=1u<<(tile&31u),bits=hot_lookup.rank.bits[index];
  if(!(bits&bit))return hot_count;
  return hot_lookup.rank.prefix[index]+hot_popcount(bits&(bit-1u));
 }
#endif
 size_t lo=0,hi=hot_count;
 while(lo<hi){size_t mid=lo+(hi-lo)/2u;if(hot_id(mid)<tile)lo=mid+1u;else hi=mid;}
 return lo<hot_count&&hot_id(lo)==tile?lo:hot_count;
}
/* Optional startup-only lease. No file access is introduced: every source
 * page must already be resident in the ordinary ROM cache. A different pack
 * or a smaller hot prefix keeps the existing reader and cold-cache behavior. */
static void hot_tag_rom_load(void){
#ifdef WF_PLATFORM_SATURN
 hot_tag_rom_ready=0;
 if(!rom_data||!rom_slots||hot_slots<3584u||sizeof tile_tags<7168u)return;
 if(!rom_preloaded)for(unsigned page=166u;page<173u;page++)
  if(rom_slot[page]==0xffffu)return;
 for(unsigned i=0;i<7u;i++){
  unsigned page=166u+i;
  const uint8_t *source=rom_preloaded?rom_data+page*1024u:
                       rom_data+(size_t)rom_slot[page]*1024u;
  memcpy((uint8_t *)tile_tags+i*1024u,source,1024u);
 }
 hot_tag_rom_ready=1;
#endif
}
wf_status_t wf_r18_hot_sprites_load(void){
 size_t count,header_bytes,payload_bytes,resident_bytes,resident_slots,dictionary_offset=0;
 uint32_t chunks=0;
 secondary_lease_reset();
 hot_tag_rom_reset();
 hot_count=hot_slots=hot_header_bytes=hot_payload_bytes=hot_dictionary_offset=hot_map_spill=0;
 hot_id_offset=8u;hot_chunk_bytes=hot_parts=0u;hot_crop_ready=hot_crop_valid=0;
#ifdef WF_PLATFORM_SATURN
 hot_rank_ready=0;
#endif
 memset(tile_valid,0,sizeof tile_valid);cold_hand=0;
 if(!read_file||!tile_data||tile_slots<2u||
    read_file("HOTSPR.BIN",0,hot_header,4096u))return WF_BLOCKED;
 unsigned dictionary=!memcmp(hot_header,"R18D",4),packed=!memcmp(hot_header,"R18P",4);
 if((!dictionary&&!packed&&memcmp(hot_header,"R18H",4))||hot_header[6]!=0u||hot_header[7]!=128u)
  return WF_BLOCKED;
 hot_id_offset=(dictionary||packed)?24u:8u;
 count=(size_t)((hot_header[4]<<8)|hot_header[5]);
 if(!count||count>(sizeof hot_header-hot_id_offset)/2u)
  return WF_BLOCKED;
 size_t id_bytes=hot_id_offset+count*2u;
 header_bytes=id_bytes>8192u?16384u:id_bytes>4096u?8192u:4096u;
 payload_bytes=count*128u;
 if(dictionary){
  uint32_t fields[4];
  for(unsigned i=0;i<4u;i++){const uint8_t *p=hot_header+8u+i*4u;fields[i]=(uint32_t)p[0]<<24|(uint32_t)p[1]<<16|(uint32_t)p[2]<<8|p[3];}
  chunks=fields[0];
  /* Mapping stride and dictionary length must identify the same width. */
  unsigned parts;
  if(fields[1]==count*8u){hot_chunk_bytes=32u;parts=4u;}
  else if(fields[1]==count*16u){hot_chunk_bytes=16u;parts=8u;}
  else if(fields[1]==count*64u){hot_chunk_bytes=4u;parts=32u;}
  else return WF_BLOCKED;
  hot_parts=parts;
  if(!chunks||chunks>65536u||chunks>count*parts||fields[2]!=chunks*hot_chunk_bytes||fields[3]!=header_bytes)
   return WF_BLOCKED;
  dictionary_offset=((size_t)fields[1]+31u)&~(size_t)31u;
  hot_map_spill=dictionary_offset>sizeof hot_indices?dictionary_offset-sizeof hot_indices:0u;
  payload_bytes=dictionary_offset+(size_t)chunks*hot_chunk_bytes;
 }
 if(packed){
  uint32_t fields[4];
  for(unsigned i=0;i<4u;i++)fields[i]=hot_packed_word(hot_header+8u+i*4u);
  if(fields[0]!=8u||fields[1]!=count*8u||fields[1]>sizeof hot_indices||
     fields[2]>count*128u||fields[3]!=header_bytes)return WF_BLOCKED;
  dictionary_offset=((size_t)fields[1]+31u)&~(size_t)31u;
  if(dictionary_offset>sizeof hot_indices)return WF_BLOCKED;
  chunks=fields[2];hot_chunk_bytes=128u;
  payload_bytes=dictionary_offset+chunks;
 }
 resident_bytes=packed?chunks:dictionary?hot_map_spill+(size_t)chunks*hot_chunk_bytes:payload_bytes;
 resident_slots=(resident_bytes+127u)/128u;
 if(resident_slots>=tile_slots)return WF_BLOCKED;
 if(id_bytes>4096u&&read_file("HOTSPR.BIN",4096u,hot_header+4096u,id_bytes-4096u))
  return WF_BLOCKED;
 for(size_t i=1;i<count;i++)if(hot_id(i)<=hot_id(i-1u))
  return WF_BLOCKED;
 if(packed){
  if(read_file("HOTSPR.BIN",(uint32_t)header_bytes,hot_indices,dictionary_offset))return WF_BLOCKED;
  size_t end=0;
  for(size_t i=0;i<count;i++){
   const uint8_t *entry=hot_indices+i*8u;uint32_t word=hot_packed_word(entry+4u);
   unsigned meta=word>>24,height=(word>>16)&255u,top=meta&31u;
   if(hot_packed_word(entry)!=end||(word&65535u)||(meta&128u)||top>16u||height>16u||top+height>16u||
      ((meta&64u)&&!(meta&32u))||(!height&&meta!=16u)||(height&&top==16u))return WF_BLOCKED;
   end+=(size_t)height*((meta&32u)?4u:8u);
   if(end>chunks)return WF_BLOCKED;
  }
  if(end!=chunks)return WF_BLOCKED;
  if(chunks&&read_file("HOTSPR.BIN",(uint32_t)(header_bytes+dictionary_offset),tile_data,chunks))return WF_BLOCKED;
 }else if(dictionary){
  size_t prefix=dictionary_offset-hot_map_spill;
  if(read_file("HOTSPR.BIN",(uint32_t)header_bytes,hot_indices,prefix))return WF_BLOCKED;
  if(hot_map_spill&&read_file("HOTSPR.BIN",(uint32_t)(header_bytes+prefix),tile_data,hot_map_spill))return WF_BLOCKED;
  for(size_t i=0;i<count*(128u/hot_chunk_bytes);i++){
   size_t offset=i*2u;
   const uint8_t *p=offset<sizeof hot_indices?hot_indices+offset:tile_data+offset-sizeof hot_indices;
   if((uint32_t)((p[0]<<8)|p[1])>=chunks)return WF_BLOCKED;
  }
  if(read_file("HOTSPR.BIN",(uint32_t)(header_bytes+dictionary_offset),tile_data+hot_map_spill,(size_t)chunks*hot_chunk_bytes))return WF_BLOCKED;
 }else if(read_file("HOTSPR.BIN",(uint32_t)header_bytes,tile_data,payload_bytes))return WF_BLOCKED;
 hot_slots=resident_slots;hot_header_bytes=header_bytes;hot_payload_bytes=payload_bytes;
 hot_dictionary_offset=dictionary_offset;
 hot_count=count;
 hot_crop_load();
 hot_rank_build();
 hot_tag_rom_load();
 return WF_OK;
}
size_t wf_r18_hot_sprites_bytes(void){
 return hot_count?hot_header_bytes+hot_payload_bytes:0u;
}
wf_status_t wf_r18_tile_read(uint16_t tile,const uint8_t **out){
 size_t lo,slot,cold;
 secondary_lease_open=0u;
 hot_crop_valid=0;
 if(!out||!tile_data||!read_file||!tile_slots)return WF_BLOCKED;
 lo=hot_find(tile);
 if(lo<hot_count){
  if(hot_chunk_bytes==128u){
   const uint8_t *entry=hot_indices+lo*8u;
   const uint8_t *source=tile_data+hot_packed_word(entry);
   uint16_t crop=(uint16_t)(hot_packed_word(entry+4u)>>16);
   if(crop==0x0010u)*out=source;
   else{
    unsigned meta=crop>>8,height=crop&255u,top=meta&31u;
    __builtin_memset(hot_tile,0,sizeof hot_tile);
    if(meta&32u)hot_packed_copy(hot_tile+top*8u+((meta&64u)?4u:0u),source,height,8u);
    else hot_packed_copy(hot_tile+top*8u,source,height*2u,4u);
    *out=hot_tile;
   }
  }else if(hot_dictionary_offset){
   unsigned parts=hot_parts;
   size_t offset=lo*parts*2u;
   const uint8_t *indices=offset<sizeof hot_indices?hot_indices+offset:tile_data+offset-sizeof hot_indices;
   for(unsigned i=0;i<parts;i++){
    unsigned index=(unsigned)indices[i*2u]<<8|indices[i*2u+1u];
    const uint8_t *source=tile_data+hot_map_spill+(size_t)index*hot_chunk_bytes;
    /* Byte stores stay inline under the SDK's -fno-builtin, without aliasing
     * byte arrays as uint32_t or making 32 library calls per tile. */
    if(hot_chunk_bytes==4u){uint8_t *destination=hot_tile+i*4u;
     destination[0]=source[0];destination[1]=source[1];
     destination[2]=source[2];destination[3]=source[3];
    }
    else if(hot_chunk_bytes==32u&&!((uintptr_t)source&3u)){
     /* The shipped dictionary uses aligned 32-byte chunks. Explicit
      * builtins keep this fixed-size copy inline under -fno-builtin;
      * the tested fallback preserves unaligned caller-owned leases. */
     __builtin_memcpy(__builtin_assume_aligned(hot_tile+i*32u,4u),
                      __builtin_assume_aligned(source,4u),32u);
    }
    else memcpy(hot_tile+i*hot_chunk_bytes,source,hot_chunk_bytes);
   }
   *out=hot_tile;
  }else *out=tile_data+lo*128u;
  hot_crop_source=tile;hot_crop_index=(uint16_t)lo;hot_crop_valid=1;
  return WF_OK;
 }
 cold=tile_slots-hot_slots;
 if(!cold)return WF_BLOCKED;
 /* The old tile%cold mapping evicted a useful tile whenever two cold IDs
  * collided, even with empty slots. Search the small cold lease and use a
  * second-chance clock; its access bits fit the existing valid array. */
 for(slot=hot_slots;slot<tile_slots;slot++)if(tile_valid[slot]&&tile_tags[slot]==tile){
  tile_valid[slot]=2u;*out=tile_data+slot*128u;return WF_OK;
 }
 /* The hand is in [0,cold): hot-load and tail-lease reset it to zero.
  * Increment/wrap preserves the exact clock policy without SH-2 division. */
 for(size_t checked=0;checked<cold;checked++){
  slot=hot_slots+cold_hand;
  if(++cold_hand==cold)cold_hand=0;
  if(!tile_valid[slot])goto cold_fill;
 }
 for(;;){
  slot=hot_slots+cold_hand;
  if(++cold_hand==cold)cold_hand=0;
  if(tile_valid[slot]==1u)break;
  tile_valid[slot]=1u;
 }
cold_fill:
#ifdef WF_PLATFORM_SATURN
 {
  int secondary=wp2015_secondary_tile_read(tile,tile_data+slot*128u);
  if(secondary<0)return WF_BLOCKED;
  if(!secondary&&read_file("SPRITES.PAK",(uint32_t)tile*128u,tile_data+slot*128u,128u))
   return WF_BLOCKED;
 }
#else
 if(read_file("SPRITES.PAK",(uint32_t)tile*128u,tile_data+slot*128u,128u))
  return WF_BLOCKED;
#endif
 tile_tags[slot]=tile;tile_valid[slot]=2u;
 *out=tile_data+slot*128u;
 return WF_OK;
}
static uint16_t be16(const uint8_t*p){return (uint16_t)((p[0]<<8)|p[1]);}
static wf_actor_raw_t *actor_link(wf_runtime_t*r,uint32_t address){return wf_actor_from_rom_ptr(r,address,NULL);}
static int add_cmd(uint16_t tile,int x,int y,unsigned fx,unsigned fy,unsigned palette){unsigned slot;
 if(x>=320||x+16<=0||y>=240||y+16<=0)return 1;
 if(plan.cmd_count==WF_VDP1_COMMANDS_MAX)return 0;
 unsigned hash=((uint32_t)tile*0x9e3779b1u)>>23;
 while((plan_tile_index[hash]>>9)==plan_tile_epoch){
  slot=((unsigned)plan_tile_index[hash]&511u)-1u;
  if(plan.tile_upload[slot].source_tile==tile)goto found;
  hash=(hash+1u)&511u;
 }
 slot=plan.tile_count;if(slot==WF_VDP1_STREAM_TILES_MAX)return 0;
 plan.tile_upload[slot]=(wf_vdp1_tile_upload_t){.source_tile=tile,.stream_slot=(uint16_t)slot};
 plan_tile_index[hash]=(uint16_t)(((unsigned)plan_tile_epoch<<9)|(slot+1u));plan.tile_count++;
found:;
 wf_vdp1_sprite_cmd_t*c=&plan.cmd[plan.cmd_count++];
 /* These three trailing bytes include chain fields and any struct padding.
  * All preceding fields are overwritten below. Keep exact zeroed bytes
  * without a freestanding memset call per visible sprite on SH-2. */
 for(size_t i=offsetof(wf_vdp1_sprite_cmd_t,chain_index);i<sizeof *c;i++)
  ((unsigned char*)c)[i]=0;
 c->x=(int16_t)x;c->y=(int16_t)y;c->stream_tile=(uint16_t)slot;c->cram_slot=(uint8_t)palette;c->flip_x=(uint8_t)fx;c->flip_y=(uint8_t)fy;return 1;}
static wf_status_t submit_records(wf_r18_video_t*v,size_t count){
 wf_status_t status=WF_OK;const wf_saturn_render_ops_t*ops=wf_saturn_yaul_render_ops();
 secondary_lease_open=0u;
 if(backup_workspace_leased)return WF_BAD_STATE;
 /* Only the counted prefixes are consumed. Every emitted command/tile is
  * fully initialized below; stale tail entries are never submitted. */
 plan.tile_count=plan.palette_count=plan.cmd_count=0;
 plan.tile_upload_bytes=plan.palette_upload_bytes=0;
 if(++plan_tile_epoch==128u){memset(plan_tile_index,0,sizeof plan_tile_index);plan_tile_epoch=1u;}
 for(size_t i=0;i<count;i++){const uint8_t*b=records[i].bytes;unsigned attr=be16(b+2);if(!(attr&1))continue;unsigned chain=(attr>>5)&7,fx=(attr>>4)&1,fy=(attr>>3)&1,palette=b[9]&15;unsigned tile=wf_sprite_number_align((unsigned)b[5]|((unsigned)b[7]<<8),chain);int x=wf_sprite_x_decode((unsigned)b[11]|((attr&4)<<6)),y=wf_sprite_y_decode((unsigned)b[1]|((attr&2)<<7));
 for(unsigned j=0;j<=chain;j++){int yy=fy?y-(int)chain*16+(int)j*16:y-(int)j*16;if(!add_cmd((uint16_t)(tile+j),x,yy,fx,fy,palette)){status=WF_BLOCKED;goto done;}}}
 uint8_t resident_bits[WF_VDP1_STREAM_TILES_MAX/8u];
 for(unsigned i=0;i<sizeof resident_bits;i++)resident_bits[i]=backend.resident[i]&backend.rom_identity[i];
 status=wf_vdp1_plan_retain_preindexed(&plan,backend.rom_source,resident_bits,WF_VDP1_STREAM_TILES_MAX,plan_tile_index,plan_tile_epoch);
 if(status!=WF_OK)goto done;
 uint16_t palette_mask=wf_r18_video_palette_pending(v,WF_R18_PALETTE_SPRITES);
 unsigned palette_count=0;for(unsigned i=0;i<16;i++)palette_count+=(palette_mask>>i)&1u;
 status=ops->begin_frame(&backend,plan.tile_count,palette_count,plan.cmd_count);if(status!=WF_OK)goto done;
 status=wf_saturn_yaul_upload_rom_tiles(&backend,plan.tile_upload,plan.tile_count,wf_r18_tile_read,hot_crop_read);
 if(status!=WF_OK)goto abort;
 for(unsigned i=0;i<16;i++)if(palette_mask&(1u<<i)){status=ops->upload_palette_xbgr444_16(&backend,(uint8_t)i,v->palettes+0x2000+i*128,32);if(status!=WF_OK)goto abort;}
 status=wf_saturn_yaul_emit_sprites(&backend,plan.cmd,plan.cmd_count);if(status!=WF_OK)goto abort;
 status=ops->end_frame(&backend);if(status!=WF_OK)goto done;
 wf_r18_video_palette_ack(v,WF_R18_PALETTE_SPRITES,palette_mask);
 last_record_count=count;
 goto done;
abort:ops->abort_frame(&backend);
done:return status;
}
wf_r18_sprite_record_t *wf_r18_present_records_buffer(size_t*capacity){
 if(capacity)*capacity=512u;
 return records;
}
wf_status_t wf_r18_present_records_commit(wf_r18_video_t*v,size_t count){
 if(!v||count>512u||busy)return WF_BAD_STATE;
 busy=1;wf_status_t st=submit_records(v,count);busy=0;return st;
}
wf_status_t wf_r18_present_batch(wf_runtime_t*r,wf_actor_raw_t*const*actors,size_t n,void*u){
 wf_r18_video_t*v=u;wf_r18_metasprite_ctx_t parser;wf_status_t status=WF_OK;
 if(!r||!v||(!actors&&n)||busy)return WF_BAD_STATE;
 busy=1;memset(&parser,0,sizeof parser);parser.read_rom=wf_r18_rom_read;parser.palette_map=v->palette_map;parser.records=records;parser.capacity=512;parser.cache=&metasprite_cache;parser.d6=r->m68k_regs.d[6];parser.d7=r->m68k_regs.d[7];
 composed_count=0;
 for(size_t i=0;i<n;i++){wf_actor_raw_t*a=actors[i];wf_actor_raw_t*partner;if(wf_u8(a,0x21)==255||(wf_be16(a,4)&0x7fff)==0x7fff)continue;partner=actor_link(r,wf_be32(a,0x26));status=wf_r18_metasprite_emit(&parser,wf_be16(a,2),wf_be16(a,4),wf_be16(a,0x14),wf_be16(a,0x16),partner?wf_be16(partner,2):0xffff);if(status!=WF_OK)goto done;if(composed_count<WF_ACTOR_SLOTS)composed[composed_count++]=a;}
 status=submit_records(v,parser.count);if(status!=WF_OK)goto done;
 /* $293E consumes a resolved link after rendering its composition. */
 for(size_t i=0;i<composed_count;i++)if((wf_u8(composed[i],0x26)&128)&&!wf_be16(composed[i],0xfe))wf_set_be32(composed[i],0x26,0);
 r->m68k_regs.d[6]=parser.d6;r->m68k_regs.d[7]=parser.d7;goto done;
done:busy=0;return status;}
size_t wf_r18_present_records_count(void){return last_record_count;}
unsigned wf_r18_present_commands(void){return plan.cmd_count;}
#ifndef WF_PLATFORM_SATURN
size_t wf_r18_present_frame_tiles(uint16_t *out,size_t capacity){
 size_t count=plan.tile_count;
 if(out){
  if(capacity<count)count=capacity;
  for(size_t i=0;i<count;i++)out[i]=plan.tile_upload[i].source_tile;
 }
 return count;
}
#endif
