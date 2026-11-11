#ifndef WF_SHARED_CROP_H
#define WF_SHARED_CROP_H
#include "wf_cropped_atlas.h"
typedef int (*wf_shared_crop_read_fn)(void*,unsigned,uint32_t,void*,size_t);
typedef int (*wf_shared_crop_decode_fn)(const uint8_t*,size_t,uint8_t*,size_t);
enum {WF_SHARED_HEADER=8192,WF_SHARED_BLOCK=16384,WF_SHARED_BLOCKS=512,
 WF_SHARED_HASH_TABLE=2116,WF_SHARED_SELECTION=10240,WF_SHARED_WINDOW=262144,
 WF_SHARED_WORKSPACE=0x54000};
static inline void wf_shared_crop_put(uint8_t*p,uint32_t value){
 p[0]=(uint8_t)(value>>24);p[1]=(uint8_t)(value>>16);p[2]=(uint8_t)(value>>8);p[3]=(uint8_t)value;
}
static inline bool wf_shared_crop_zero(const uint8_t*p,size_t first,size_t end){
 for(size_t i=first;i<end;i++)if(p[i])return false;
 return true;
}
static inline bool wf_shared_crop_disjoint(const void*a,size_t an,const void*b,size_t bn){
 uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;
 if(an>UINTPTR_MAX-x||bn>UINTPTR_MAX-y)return false;
 return x+an<=y||y+bn<=x;
}
typedef struct {
 wf_shared_crop_read_fn read;void*user;uint8_t*data;
 uint32_t payload,base,available;
} wf_shared_crop_window_t;
static inline bool wf_shared_crop_fetch(wf_shared_crop_window_t*w,uint32_t base){
 if(base>=w->payload||(base&(WF_SHARED_WINDOW-1u)))return false;
 if(w->base==base)return true;
 uint32_t n=w->payload-base;if(n>WF_SHARED_WINDOW)n=WF_SHARED_WINDOW;
 if(w->read(w->user,0,WF_SHARED_HEADER+base,w->data,n))return false;
 w->base=base;w->available=n;return true;
}
/* All scratch storage lives after the declared atlas, inside the supplied
 * cart lease. The callback reads pool(kind0) or selector(kind1) bytes. The
 * context remains empty on every failure; no partial atlas is published. */
static inline bool wf_shared_crop_load(wf_cropped_atlas_t *out,void *cart,size_t capacity,
 const uint8_t*hints,size_t hint_bytes,unsigned mask,unsigned royal,
 uint32_t pool_bytes,uint32_t selector_bytes,wf_shared_crop_read_fn read,void*user,
 wf_shared_crop_decode_fn decode,void*fast_scratch,size_t fast_bytes){
 if(out)*out=(wf_cropped_atlas_t){0};
 if(!out||!cart||!hints||!read||!decode||((uintptr_t)cart&31u)||
    capacity>0x400000u||(capacity&31u)||capacity<WF_SHARED_WORKSPACE+WF_CROPPED_ATLAS_MAP_BYTES||
    hint_bytes!=WF_CROPPED_ATLAS_HINT_BYTES||!mask||mask>0xfffu||royal>1u||
    pool_bytes<WF_SHARED_HEADER||selector_bytes!=WF_SHARED_SELECTION)return false;
 if(fast_scratch){
  if(((uintptr_t)fast_scratch&31u)||fast_bytes<49152u||
     !wf_shared_crop_disjoint(fast_scratch,49152u,cart,capacity)||
     !wf_shared_crop_disjoint(fast_scratch,49152u,hints,hint_bytes))return false;
 }else if(fast_bytes)return false;
 uint8_t*data=cart;size_t limit=capacity-WF_SHARED_WORKSPACE;
 uint8_t*header=data+limit;
 uint8_t*selection=header+8192u;
 uint8_t*decoded=fast_scratch?fast_scratch:header+0x4800u;
 uint8_t*bridge=fast_scratch?(uint8_t*)fast_scratch+16384u:header+0x8800u;
 uint8_t*input=header+0x10800u;
 _Static_assert(0x10800u+WF_SHARED_WINDOW<=WF_SHARED_WORKSPACE,"Shared pool scratch lease");
 if(read(user,1,0,selection,WF_SHARED_SELECTION)||read(user,0,0,header,WF_SHARED_HEADER))return false;
 if(memcmp(selection,"R20S",4)||wf_cropped_atlas_word(selection+4)!=1u||
    wf_cropped_atlas_word(selection+8)!=2048u||wf_cropped_atlas_word(selection+12)!=mask||
    wf_cropped_atlas_word(selection+16)!=royal||wf_cropped_atlas_word(selection+44)!=65536u||
    !wf_shared_crop_zero(selection,48u,2044u)||
    wf_cropped_atlas_hash(selection,2044u)!=wf_cropped_atlas_word(selection+2044u)||
    wf_cropped_atlas_hash(selection+2048u,8192u)!=wf_cropped_atlas_word(selection+40u))return false;
 uint32_t wanted_count=wf_cropped_atlas_word(selection+20),memory=wf_cropped_atlas_word(selection+24);
 uint32_t memory_hash=wf_cropped_atlas_word(selection+28),hint_hash=wf_cropped_atlas_hash(hints,hint_bytes);
 if(!wanted_count||wanted_count>65536u||memory<WF_CROPPED_ATLAS_MAP_BYTES||memory>limit||
    (memory&3u)||wf_cropped_atlas_word(selection+32)!=hint_hash)return false;
 if(memcmp(header,"R20G",4)||wf_cropped_atlas_word(header+4)!=1u||
    wf_cropped_atlas_word(header+8)!=WF_SHARED_HEADER||wf_cropped_atlas_word(header+12)!=65536u||
    wf_cropped_atlas_word(header+16)!=WF_SHARED_BLOCK||wf_cropped_atlas_word(header+32)!=hint_hash||
    wf_cropped_atlas_word(header+40)!=1u||!wf_shared_crop_zero(header,44u,64u)||
    wf_cropped_atlas_hash(header,8188u)!=wf_cropped_atlas_word(header+8188u)||
    wf_cropped_atlas_word(selection+36)!=wf_cropped_atlas_word(header+8188u))return false;
 uint32_t blocks=wf_cropped_atlas_word(header+20),raw=wf_cropped_atlas_word(header+24),packed=wf_cropped_atlas_word(header+28);
 if(!blocks||blocks>WF_SHARED_BLOCKS||raw<=(blocks-1u)*WF_SHARED_BLOCK||raw>blocks*WF_SHARED_BLOCK||
    packed!=pool_bytes-WF_SHARED_HEADER||
    wf_cropped_atlas_word(header+64)||wf_cropped_atlas_word(header+64+blocks*4u)!=packed||
    !wf_shared_crop_zero(header,64u+(blocks+1u)*4u,WF_SHARED_HASH_TABLE)||
    !wf_shared_crop_zero(header,WF_SHARED_HASH_TABLE+blocks*4u,8188u))return false;
 for(unsigned i=0;i<blocks;i++){
  uint32_t first=wf_cropped_atlas_word(header+64u+i*4u),last=wf_cropped_atlas_word(header+68u+i*4u);
  if(last<=first||last-first>32768u)return false;
 }
 uint32_t cursor=WF_CROPPED_ATLAS_MAP_BYTES,total=0,count=0;
 memset(data,0,WF_CROPPED_ATLAS_MAP_BYTES);
 for(unsigned tile=0;tile<65536u;tile++){
  uint16_t crop=(uint16_t)((unsigned)hints[tile*2u]<<8|hints[tile*2u+1u]);
  if(!wf_cropped_atlas_shape(crop))return false;
  unsigned bytes=(crop&255u)*((crop&0x2000u)?4u:8u);total+=bytes;
  if(selection[2048u+(tile>>3)]&(1u<<(tile&7u))){
   if(bytes>memory-cursor)return false;
   wf_shared_crop_put(data+tile*4u,cursor+1u);cursor+=bytes;count++;
  }
 }
 if(total!=raw||cursor!=memory||count!=wanted_count)return false;
 wf_shared_crop_window_t window={read,user,input,packed,UINT32_MAX,0};
 uint32_t global_hash=2166136261u;unsigned tile=0,tile_at=0;
 for(unsigned block=0;block<blocks;block++){
  uint32_t first=wf_cropped_atlas_word(header+64u+block*4u),last=wf_cropped_atlas_word(header+68u+block*4u);
  uint32_t base=first&~(WF_SHARED_WINDOW-1u),length=last-first;
  if(!wf_shared_crop_fetch(&window,base))return false;
  const uint8_t*source=input+(first-base);
  if(last-base>window.available){
   uint32_t prefix=window.available-(first-base);
   memcpy(bridge,source,prefix);
   if(!wf_shared_crop_fetch(&window,base+WF_SHARED_WINDOW)||length-prefix>window.available)return false;
   memcpy(bridge+prefix,input,length-prefix);source=bridge;
  }else if(fast_scratch){
   /* Decode and dictionary matches use the caller's fast temporary lease.
    * The cartridge still owns the immutable stream window and final atlas. */
   memcpy(bridge,source,length);source=bridge;
  }
  uint32_t bytes=raw-block*WF_SHARED_BLOCK;if(bytes>WF_SHARED_BLOCK)bytes=WF_SHARED_BLOCK;
  if(decode(source,length,decoded,bytes)||wf_cropped_atlas_hash(decoded,bytes)!=wf_cropped_atlas_word(header+WF_SHARED_HASH_TABLE+block*4u))return false;
  for(unsigned i=0;i<bytes;i++)global_hash=(global_hash^decoded[i])*16777619u;
  unsigned at=0;
  while(at<bytes){
   unsigned size=0;
   while(tile<65536u){
    uint16_t crop=(uint16_t)((unsigned)hints[tile*2u]<<8|hints[tile*2u+1u]);
    size=(crop&255u)*((crop&0x2000u)?4u:8u);
    if(tile_at<size)break;
    tile++;tile_at=0;
   }
   if(tile==65536u)return false;
   unsigned take=size-tile_at;if(take>bytes-at)take=bytes-at;
   uint32_t entry=wf_cropped_atlas_word(data+tile*4u);
   if(entry)memcpy(data+(entry-1u)+tile_at,decoded+at,take);
   tile_at+=take;at+=take;
  }
 }
 if(global_hash!=wf_cropped_atlas_word(header+36u))return false;
 /* Reuse the already-proved R19C validator for final canonical map/hash
  * checks. Selection bits are no longer needed at this point. */
 memset(selection,0,2048u);memcpy(selection,"R19C",4);
 wf_shared_crop_put(selection+4,2048u);wf_shared_crop_put(selection+8,memory);
 wf_shared_crop_put(selection+12,65536u);wf_shared_crop_put(selection+16,count);
 wf_shared_crop_put(selection+20,memory-WF_CROPPED_ATLAS_MAP_BYTES);
 wf_shared_crop_put(selection+24,memory_hash);wf_shared_crop_put(selection+28,hint_hash);
 return wf_cropped_atlas_bind(out,selection,2048u,data,memory,hints,hint_bytes);
}
#endif
