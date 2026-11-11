#ifndef WF_FULL_CROP_LZ4_H
#define WF_FULL_CROP_LZ4_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

/* R24L v1: immutable, complete original-ID map and independently bounded LZ4
 * blocks. Caller owns all storage. A slot argument promises 16384 writable
 * bytes; portable C cannot probe operating-system write permissions. */
enum {
 WF_FULL_CROP_HEADER=2048, WF_FULL_CROP_TILES=65536,
 WF_FULL_CROP_MAP_BYTES=196608, WF_FULL_CROP_HINT_BYTES=131072,
 WF_FULL_CROP_BLOCK=16384, WF_FULL_CROP_MAX_BLOCKS=512,
 WF_FULL_CROP_MAX_SLOTS=18, WF_FULL_CROP_MAX_ASSET=4194304,
 WF_FULL_CROP_MAX_VIRTUAL=8388608, WF_FULL_CROP_PACKED=0x8000
};
enum {
 WF_FULL_CROP_ERROR_NONE=0, WF_FULL_CROP_ERROR_ARGUMENT,
 WF_FULL_CROP_ERROR_HEADER, WF_FULL_CROP_ERROR_CHECKSUM,
 WF_FULL_CROP_ERROR_SHAPE, WF_FULL_CROP_ERROR_MAP,
 WF_FULL_CROP_ERROR_DIRECTORY, WF_FULL_CROP_ERROR_ALIAS,
 WF_FULL_CROP_ERROR_DECODE, WF_FULL_CROP_ERROR_INACTIVE
};
typedef int (*wf_full_crop_decode_fn)(const uint8_t*,size_t,uint8_t*,size_t);
typedef struct {
 bool active;
 uint32_t error,last_tile,last_block,hits,misses,decodes,decoded_bytes;
 const uint8_t *asset,*hints,*directory,*payload;
 size_t asset_bytes;
 uint32_t virtual_bytes,payload_bytes,block_count;
 wf_full_crop_decode_fn decode;
 uint8_t *slots[WF_FULL_CROP_MAX_SLOTS];
 uint32_t tags[WF_FULL_CROP_MAX_SLOTS];
 /* Exact recency ranks, not a wrapping clock: 0 is most recently used. */
 uint8_t ranks[WF_FULL_CROP_MAX_SLOTS],slot_count;
 uint8_t bridge[128];
} wf_full_crop_lz4_t;
static inline uint32_t wf_full_crop_word(const uint8_t *p) {
 return (uint32_t)p[0]<<24|(uint32_t)p[1]<<16|(uint32_t)p[2]<<8|p[3];
}
static inline uint32_t wf_full_crop_hash(const uint8_t *p,size_t n) {
 uint32_t h=2166136261u;while(n--)h=(h^*p++)*16777619u;return h;
}
static inline bool wf_full_crop_range(const void *p,size_t n) {
 return p&&n<=UINTPTR_MAX-(uintptr_t)p;
}
static inline bool wf_full_crop_disjoint(const void *a,size_t an,const void *b,size_t bn) {
 if(!wf_full_crop_range(a,an)||!wf_full_crop_range(b,bn))return false;
 uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;return x+an<=y||y+bn<=x;
}
static inline bool wf_full_crop_zero(const uint8_t *p,size_t first,size_t last) {
 for(size_t i=first;i<last;i++)if(p[i])return false;
 return true;
}
static inline bool wf_full_crop_shape(uint16_t crop) {
 unsigned top=(crop>>8)&31u,height=crop&255u;
 return !(crop&0x8000u)&&top<=16u&&height<=16u&&top+height<=16u&&
        (!(crop&0x4000u)||(crop&0x2000u));
}
static inline uint16_t wf_full_crop_hint(const uint8_t *h,unsigned tile) {
 return (uint16_t)((unsigned)h[tile*2u]<<8|h[tile*2u+1u]);
}
static inline uint32_t wf_full_crop_map(const uint8_t *a,unsigned tile) {
 const uint8_t *p=a+WF_FULL_CROP_HEADER+tile*3u;
 return (uint32_t)p[0]<<16|(uint32_t)p[1]<<8|p[2];
}
static inline void wf_full_crop_reset(wf_full_crop_lz4_t *c) {
 if(!c)return;
 *c=(wf_full_crop_lz4_t){0};
 c->last_tile=UINT32_MAX;c->last_block=UINT32_MAX;
 for(unsigned i=0;i<WF_FULL_CROP_MAX_SLOTS;i++)c->tags[i]=UINT32_MAX;
}
/* Header-only sizing: body is not accessed. A successful result permits the
 * loader to read exactly total_bytes, never beyond the physical cart lease. */
static inline bool wf_full_crop_extent(const uint8_t *h,size_t header_bytes,
 size_t cart_capacity,size_t *total_bytes) {
 if(total_bytes)*total_bytes=0;
 if(!total_bytes||header_bytes<WF_FULL_CROP_HEADER||
    !wf_full_crop_range(h,WF_FULL_CROP_HEADER)||memcmp(h,"R24L",4)||
    wf_full_crop_word(h+4)!=1u||wf_full_crop_word(h+8)!=WF_FULL_CROP_HEADER||
    wf_full_crop_word(h+16)!=WF_FULL_CROP_TILES||
    wf_full_crop_word(h+20)!=WF_FULL_CROP_HEADER||
    wf_full_crop_word(h+24)!=WF_FULL_CROP_MAP_BYTES||wf_full_crop_word(h+28)!=3u||
    wf_full_crop_word(h+32)!=WF_FULL_CROP_BLOCK||wf_full_crop_word(h+44)!=8u||
    wf_full_crop_word(h+64)!=WF_FULL_CROP_HINT_BYTES||
    wf_full_crop_word(h+76)!=8388608u||!wf_full_crop_zero(h,80,2044)||
    wf_full_crop_hash(h,2044)!=wf_full_crop_word(h+2044))return false;
 size_t total=wf_full_crop_word(h+12),blocks=wf_full_crop_word(h+36);
 size_t d=wf_full_crop_word(h+40),dc=wf_full_crop_word(h+48);
 size_t p=wf_full_crop_word(h+52),pb=wf_full_crop_word(h+56),v=wf_full_crop_word(h+60);
 if(!v||v>WF_FULL_CROP_MAX_VIRTUAL||!blocks||blocks>WF_FULL_CROP_MAX_BLOCKS||
    blocks!=(v+WF_FULL_CROP_BLOCK-1u)/WF_FULL_CROP_BLOCK||dc!=blocks+1u||
    d!=WF_FULL_CROP_HEADER+WF_FULL_CROP_MAP_BYTES||
    p!=((d+dc*8u+31u)&~(size_t)31u)||!pb||pb>WF_FULL_CROP_MAX_ASSET||
    total!=((p+pb+31u)&~(size_t)31u)||
    total>WF_FULL_CROP_MAX_ASSET||total>cart_capacity)return false;
 *total_bytes=total;return true;
}
static inline bool wf_full_crop_fail(wf_full_crop_lz4_t *c,uint32_t error) {
 c->error=error;return false;
}
/* Every bind, including a rejected rebind, revokes all old pointers and tags.
 * Checksums cover the immutable source once. Directory decoded hashes are
 * host-proof metadata, deliberately not a per-miss hot-path checksum scan. */
static inline bool wf_full_crop_bind(wf_full_crop_lz4_t *c,const uint8_t *asset,
 size_t asset_bytes,const uint8_t *hints,size_t hint_bytes,
 uint8_t *const *slots,size_t slot_count,wf_full_crop_decode_fn decode) {
 if(!c)return false;
 wf_full_crop_reset(c);
 if(!decode||!slots||!slot_count||slot_count>WF_FULL_CROP_MAX_SLOTS||
    hint_bytes!=WF_FULL_CROP_HINT_BYTES||asset_bytes>WF_FULL_CROP_MAX_ASSET||
    !wf_full_crop_range(asset,asset_bytes)||!wf_full_crop_range(hints,hint_bytes)||
    !wf_full_crop_range(slots,slot_count*sizeof(*slots)))
  return wf_full_crop_fail(c,WF_FULL_CROP_ERROR_ARGUMENT);
 if(!wf_full_crop_disjoint(asset,asset_bytes,c,sizeof(*c))||
    !wf_full_crop_disjoint(hints,hint_bytes,c,sizeof(*c))||
    !wf_full_crop_disjoint(slots,slot_count*sizeof(*slots),c,sizeof(*c)))
  return wf_full_crop_fail(c,WF_FULL_CROP_ERROR_ALIAS);
 size_t expected=0;
 if(!wf_full_crop_extent(asset,asset_bytes,asset_bytes,&expected)||expected!=asset_bytes)
  return wf_full_crop_fail(c,WF_FULL_CROP_ERROR_HEADER);
 for(size_t i=0;i<slot_count;i++) {
  if(((uintptr_t)slots[i]&31u)||
     !wf_full_crop_disjoint(slots[i],WF_FULL_CROP_BLOCK,asset,asset_bytes)||
     !wf_full_crop_disjoint(slots[i],WF_FULL_CROP_BLOCK,hints,hint_bytes)||
     !wf_full_crop_disjoint(slots[i],WF_FULL_CROP_BLOCK,c,sizeof(*c)))
   return wf_full_crop_fail(c,WF_FULL_CROP_ERROR_ALIAS);
  for(size_t j=0;j<i;j++)if(!wf_full_crop_disjoint(slots[i],WF_FULL_CROP_BLOCK,slots[j],WF_FULL_CROP_BLOCK))
   return wf_full_crop_fail(c,WF_FULL_CROP_ERROR_ALIAS);
 }
 if(wf_full_crop_hash(asset+WF_FULL_CROP_HEADER,asset_bytes-WF_FULL_CROP_HEADER)!=wf_full_crop_word(asset+72)||
    wf_full_crop_hash(hints,hint_bytes)!=wf_full_crop_word(asset+68))
  return wf_full_crop_fail(c,WF_FULL_CROP_ERROR_CHECKSUM);
 uint32_t blocks=wf_full_crop_word(asset+36),d=wf_full_crop_word(asset+40);
 uint32_t p=wf_full_crop_word(asset+52),pb=wf_full_crop_word(asset+56),v=wf_full_crop_word(asset+60);
 if(!wf_full_crop_zero(asset,d+(blocks+1u)*8u,p)||
    !wf_full_crop_zero(asset,(size_t)p+pb,asset_bytes))
  return wf_full_crop_fail(c,WF_FULL_CROP_ERROR_HEADER);
 const uint8_t *directory=asset+d;
 if(wf_full_crop_word(directory)||wf_full_crop_word(directory+blocks*8u)!=pb||
    wf_full_crop_word(directory+blocks*8u+4u))
  return wf_full_crop_fail(c,WF_FULL_CROP_ERROR_DIRECTORY);
 for(uint32_t i=0;i<blocks;i++) {
  uint32_t first=wf_full_crop_word(directory+i*8u),last=wf_full_crop_word(directory+(i+1u)*8u);
  if(first>=last||last>pb||last-first>65536u)
   return wf_full_crop_fail(c,WF_FULL_CROP_ERROR_DIRECTORY);
 }
 for(unsigned tile=0;tile<WF_FULL_CROP_TILES;tile++) {
  uint16_t crop=wf_full_crop_hint(hints,tile);
  if(!wf_full_crop_shape(crop))return wf_full_crop_fail(c,WF_FULL_CROP_ERROR_SHAPE);
  uint32_t entry=wf_full_crop_map(asset,tile),length=(crop&255u)*((crop&0x2000u)?4u:8u);
  if(!entry||entry-1u>v||length>v-(entry-1u))
   return wf_full_crop_fail(c,WF_FULL_CROP_ERROR_MAP);
 }
 c->asset=asset;c->asset_bytes=asset_bytes;c->hints=hints;
 c->directory=directory;c->payload=asset+p;c->payload_bytes=pb;
 c->virtual_bytes=v;c->block_count=blocks;c->decode=decode;
 c->slot_count=(uint8_t)slot_count;
 for(size_t i=0;i<slot_count;i++)c->slots[i]=slots[i];
 c->active=true;return true;
}
static inline void wf_full_crop_count(uint32_t *value,uint32_t add) {
 *value=add>UINT32_MAX-*value?UINT32_MAX:*value+add;
}
static inline uint8_t *wf_full_crop_acquire(wf_full_crop_lz4_t *c,uint32_t block) {
 c->last_block=block;
 unsigned chosen=c->slot_count;
 for(unsigned i=0;i<c->slot_count;i++)if(c->tags[i]==block){chosen=i;break;}
 if(chosen<c->slot_count)wf_full_crop_count(&c->hits,1);
 else {
  for(unsigned i=0;i<c->slot_count;i++)if(c->tags[i]==UINT32_MAX){chosen=i;break;}
  if(chosen==c->slot_count) {
   chosen=0;for(unsigned i=1;i<c->slot_count;i++)if(c->ranks[i]>c->ranks[chosen])chosen=i;
  }
  wf_full_crop_count(&c->misses,1);
  /* All spans and output lengths were checked before publishing the binding. */
  uint32_t first=wf_full_crop_word(c->directory+block*8u),last=wf_full_crop_word(c->directory+(block+1u)*8u);
  uint32_t bytes=c->virtual_bytes-block*WF_FULL_CROP_BLOCK;
  if(bytes>WF_FULL_CROP_BLOCK)bytes=WF_FULL_CROP_BLOCK;
  wf_full_crop_count(&c->decodes,1);
  if(c->decode(c->payload+first,last-first,c->slots[chosen],bytes)) {
   c->active=false;c->error=WF_FULL_CROP_ERROR_DECODE;
   /* Retain failing tile/block/counters, revoke all source/cache capabilities. */
   c->asset=NULL;c->hints=NULL;c->directory=NULL;c->payload=NULL;c->decode=NULL;
   for(unsigned i=0;i<c->slot_count;i++) { c->slots[i]=NULL;c->tags[i]=UINT32_MAX; }
   return NULL;
  }
  wf_full_crop_count(&c->decoded_bytes,bytes);
 }
 unsigned old=c->tags[chosen]==UINT32_MAX?c->slot_count:c->ranks[chosen];
 for(unsigned i=0;i<c->slot_count;i++)if(i!=chosen&&c->tags[i]!=UINT32_MAX&&c->ranks[i]<old)c->ranks[i]++;
 c->ranks[chosen]=0;c->tags[chosen]=block;return c->slots[chosen];
}
/* The returned span lives until the next get/reset/rebind. Cross-block crops
 * copy their first fragment before the second acquisition can evict it, so
 * even one cache slot is sufficient. Empty crops return a stable safe pointer. */
static inline bool wf_full_crop_get(wf_full_crop_lz4_t *c,uint16_t tile,
 const uint8_t **out,uint16_t *crop) {
 if(out)*out=NULL;
 if(crop)*crop=0xffffu;
 if(!c)return false;
 if(!c->active) { if(!c->error)c->error=WF_FULL_CROP_ERROR_INACTIVE;return false; }
 if(!out||!crop)return wf_full_crop_fail(c,WF_FULL_CROP_ERROR_ARGUMENT);
 c->last_tile=tile;c->last_block=UINT32_MAX;c->error=WF_FULL_CROP_ERROR_NONE;
 uint16_t sh=wf_full_crop_hint(c->hints,tile);
 uint32_t length=(sh&255u)*((sh&0x2000u)?4u:8u),off=wf_full_crop_map(c->asset,tile)-1u;
 const uint8_t *result=c->bridge;
 if(length) {
  uint32_t block=off/WF_FULL_CROP_BLOCK,within=off%WF_FULL_CROP_BLOCK;
  uint8_t *data=wf_full_crop_acquire(c,block);if(!data)return false;
  uint32_t first=WF_FULL_CROP_BLOCK-within;
  if(length<=first)result=data+within;
  else {
   memcpy(c->bridge,data+within,first);
   data=wf_full_crop_acquire(c,block+1u);if(!data)return false;
   memcpy(c->bridge+first,data,length-first);result=c->bridge;
  }
 }
 *out=result;*crop=(uint16_t)(sh|WF_FULL_CROP_PACKED);return true;
}
#endif
