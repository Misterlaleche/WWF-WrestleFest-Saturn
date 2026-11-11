#ifndef WF_GROUPED_CROP_H
#define WF_GROUPED_CROP_H
#include "wf_cropped_atlas.h"
typedef int (*wf_grouped_crop_read_fn)(void*,unsigned,uint32_t,void*,size_t);
enum{WF_GROUPED_HEADER=16384,WF_GROUPED_BLOCK=16384,WF_GROUPED_BLOCKS=512,
 WF_GROUPED_SELECTION=10240,WF_GROUPED_WINDOW=262144,WF_GROUPED_WORKSPACE=0x48000};
static inline void wf_grouped_crop_put(uint8_t*p,uint32_t v){
 p[0]=(uint8_t)(v>>24);p[1]=(uint8_t)(v>>16);p[2]=(uint8_t)(v>>8);p[3]=(uint8_t)v;
}
static inline bool wf_grouped_crop_zero(const uint8_t*p,size_t a,size_t b){
 for(size_t i=a;i<b;i++)if(p[i])return false;
 return true;
}
static inline bool wf_grouped_crop_disjoint(const void*a,size_t an,const void*b,size_t bn){
 uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;
 if(an>UINTPTR_MAX-x||bn>UINTPTR_MAX-y)return false;
 return x+an<=y||y+bn<=x;
}
static inline bool wf_grouped_crop_bit(const uint8_t*p,unsigned i){return(p[i>>3]&(1u<<(i&7u)))!=0;}
/* The pool is immutable. Validate only selected blocks at runtime; the encoder
 * validates the whole pool. No partial atlas is published. The fast lease is
 * temporary and must not overlap the cartridge, hints or output context. */
static inline bool wf_grouped_crop_load(wf_cropped_atlas_t*out,void*cart,size_t capacity,
 const uint8_t*hints,size_t hint_bytes,unsigned mask,unsigned royal,
 uint32_t pool_bytes,uint32_t selector_bytes,wf_grouped_crop_read_fn read,void*user,
 void*fast_scratch,size_t fast_bytes){
 if(out)*out=(wf_cropped_atlas_t){0};
 if(!out||!cart||!hints||!read||!fast_scratch||((uintptr_t)cart&31u)||
    capacity>0x400000u||(capacity&31u)||capacity<WF_GROUPED_WORKSPACE+WF_CROPPED_ATLAS_MAP_BYTES||
    hint_bytes!=WF_CROPPED_ATLAS_HINT_BYTES||!mask||mask>0xfffu||royal>1u||
    pool_bytes<WF_GROUPED_HEADER||(pool_bytes&2047u)||selector_bytes!=WF_GROUPED_SELECTION||
    ((uintptr_t)fast_scratch&31u)||fast_bytes<WF_GROUPED_BLOCK||
    !wf_grouped_crop_disjoint(cart,capacity,hints,hint_bytes)||
    !wf_grouped_crop_disjoint(cart,capacity,out,sizeof*out)||
    !wf_grouped_crop_disjoint(fast_scratch,WF_GROUPED_BLOCK,cart,capacity)||
    !wf_grouped_crop_disjoint(fast_scratch,WF_GROUPED_BLOCK,hints,hint_bytes)||
    !wf_grouped_crop_disjoint(fast_scratch,WF_GROUPED_BLOCK,out,sizeof*out))return false;
 uint8_t*data=cart;size_t limit=capacity-WF_GROUPED_WORKSPACE;
 uint8_t*header=data+limit,*selection=header+WF_GROUPED_HEADER,*input=header+0x6800u,*block=fast_scratch;
 _Static_assert(0x6800u+WF_GROUPED_WINDOW<=WF_GROUPED_WORKSPACE,"Grouped pool scratch lease");
 if(read(user,1,0,selection,WF_GROUPED_SELECTION)||read(user,0,0,header,WF_GROUPED_HEADER))return false;
 if(memcmp(selection,"R21S",4)||wf_cropped_atlas_word(selection+4)!=1u||
    wf_cropped_atlas_word(selection+8)!=2048u||wf_cropped_atlas_word(selection+12)!=mask||
    wf_cropped_atlas_word(selection+16)!=royal||wf_cropped_atlas_word(selection+44)!=65536u||
    wf_cropped_atlas_word(selection+60)||!wf_grouped_crop_zero(selection,128u,2044u)||
    wf_cropped_atlas_hash(selection,2044u)!=wf_cropped_atlas_word(selection+2044u)||
    wf_cropped_atlas_hash(selection+2048u,8192u)!=wf_cropped_atlas_word(selection+40u)||
    wf_cropped_atlas_hash(selection+64u,64u)!=wf_cropped_atlas_word(selection+56u))return false;
 uint32_t wanted=wf_cropped_atlas_word(selection+20),memory=wf_cropped_atlas_word(selection+24);
 uint32_t memory_hash=wf_cropped_atlas_word(selection+28),hint_hash=wf_cropped_atlas_hash(hints,hint_bytes);
 if(!wanted||wanted>65536u||memory<WF_CROPPED_ATLAS_MAP_BYTES||memory>limit||(memory&3u)||
    wf_cropped_atlas_word(selection+32)!=hint_hash)return false;
 if(memcmp(header,"R21R",4)||wf_cropped_atlas_word(header+4)!=1u||
    wf_cropped_atlas_word(header+8)!=WF_GROUPED_HEADER||wf_cropped_atlas_word(header+12)!=65536u||
    wf_cropped_atlas_word(header+16)!=WF_GROUPED_BLOCK||wf_cropped_atlas_word(header+32)!=pool_bytes||
    wf_cropped_atlas_word(header+36)!=hint_hash||wf_cropped_atlas_word(header+40)!=8256u||
    !wf_grouped_crop_zero(header,44u,64u)||
    wf_cropped_atlas_hash(header,16380u)!=wf_cropped_atlas_word(header+16380u)||
    wf_cropped_atlas_word(selection+36)!=wf_cropped_atlas_word(header+16380u))return false;
 uint32_t blocks=wf_cropped_atlas_word(header+20),records=0,pixels=0,end=WF_GROUPED_HEADER,needed=0;
 if(!blocks||blocks>WF_GROUPED_BLOCKS||wf_cropped_atlas_word(selection+52)!=blocks||
    !wf_grouped_crop_zero(header,64u+blocks*16u,16380u))return false;
 for(unsigned i=0;i<WF_GROUPED_BLOCKS;i++){
  if(wf_grouped_crop_bit(selection+64u,i)){if(i>=blocks)return false;needed++;}
  if(i>=blocks)continue;
  const uint8_t*e=header+64u+i*16u;
  uint32_t offset=wf_cropped_atlas_word(e),bytes=wf_cropped_atlas_word(e+4),count=wf_cropped_atlas_word(e+8);
  if(offset!=end||bytes<16u||bytes>WF_GROUPED_BLOCK||(bytes&3u)||!count||count>2047u||
     bytes<8u+count*8u||bytes>8u+count*132u)return false;
  uint32_t rounded=(bytes+2047u)&~2047u;
  if(end>pool_bytes||rounded>pool_bytes-end)return false;
  end+=rounded;records+=count;pixels+=bytes-8u-count*4u;
 }
 if(end!=pool_bytes||records!=wf_cropped_atlas_word(header+24)||pixels!=wf_cropped_atlas_word(header+28)||
    needed!=wf_cropped_atlas_word(selection+48))return false;
 uint32_t cursor=WF_CROPPED_ATLAS_MAP_BYTES,count=0,total=0,nonempty=0;
 memset(data,0,WF_CROPPED_ATLAS_MAP_BYTES);uint8_t*pending=selection+2048u;
 for(unsigned tile=0;tile<65536u;tile++){
  uint16_t crop=(uint16_t)((unsigned)hints[tile*2u]<<8|hints[tile*2u+1u]);
  if(!wf_cropped_atlas_shape(crop))return false;
  unsigned bytes=(crop&255u)*((crop&0x2000u)?4u:8u);total+=bytes;if(bytes)nonempty++;
  if(wf_grouped_crop_bit(pending,tile)){
   if(bytes>memory-cursor)return false;
   wf_grouped_crop_put(data+tile*4u,cursor+1u);cursor+=bytes;count++;
   if(!bytes)pending[tile>>3]&=(uint8_t)~(1u<<(tile&7u));
  }
 }
 if(cursor!=memory||count!=wanted||total!=pixels||nonempty!=records)return false;
 for(unsigned first=0;first<blocks;){
  if(!wf_grouped_crop_bit(selection+64u,first)){first++;continue;}
  const uint8_t*entry=header+64u+first*16u;
  uint32_t start=wf_cropped_atlas_word(entry),stop=start+((wf_cropped_atlas_word(entry+4)+2047u)&~2047u);
  unsigned last=first;
  for(unsigned i=first+1u;i<blocks;i++){
   if(!wf_grouped_crop_bit(selection+64u,i))continue;
   const uint8_t*e=header+64u+i*16u;uint32_t at=wf_cropped_atlas_word(e),next=at+((wf_cropped_atlas_word(e+4)+2047u)&~2047u);
   if(at-stop>32768u||next-start>WF_GROUPED_WINDOW)break;
   last=i;stop=next;
  }
  if(read(user,0,start,input,stop-start))return false;
  for(unsigned i=first;i<=last;i++){
   if(!wf_grouped_crop_bit(selection+64u,i))continue;
   const uint8_t*e=header+64u+i*16u;uint32_t at=wf_cropped_atlas_word(e),bytes=wf_cropped_atlas_word(e+4);
   uint32_t n=wf_cropped_atlas_word(e+8),rounded=(bytes+2047u)&~2047u;
   const uint8_t*source=input+at-start;
   if(!wf_grouped_crop_zero(source,bytes,rounded))return false;
   memcpy(block,source,bytes);
   if(wf_cropped_atlas_hash(block,bytes)!=wf_cropped_atlas_word(e+12)||wf_cropped_atlas_word(block)!=n)return false;
   uint32_t used=8u,block_pixels=0,selected=0;
   for(unsigned record=0;record<n;record++){
    if(bytes-used<4u)return false;
    unsigned tile=(unsigned)block[used]<<8|block[used+1u],length=(unsigned)block[used+2u]<<8|block[used+3u];used+=4u;
    uint16_t crop=(uint16_t)((unsigned)hints[tile*2u]<<8|hints[tile*2u+1u]);
    unsigned want=(crop&255u)*((crop&0x2000u)?4u:8u);
    if(!length||length!=want||length>bytes-used)return false;
    uint32_t target=wf_cropped_atlas_word(data+tile*4u);
    if(target){
     if(!wf_grouped_crop_bit(pending,tile)||target-1u>memory||length>memory-(target-1u))return false;
     memcpy(data+target-1u,block+used,length);pending[tile>>3]&=(uint8_t)~(1u<<(tile&7u));selected++;
    }
    used+=length;block_pixels+=length;
   }
   if(used!=bytes||block_pixels!=wf_cropped_atlas_word(block+4)||!selected)return false;
  }
  first=last+1u;
 }
 if(!wf_grouped_crop_zero(pending,0,8192u))return false;
 memset(selection,0,2048u);memcpy(selection,"R19C",4);
 wf_grouped_crop_put(selection+4,2048u);wf_grouped_crop_put(selection+8,memory);
 wf_grouped_crop_put(selection+12,65536u);wf_grouped_crop_put(selection+16,count);
 wf_grouped_crop_put(selection+20,memory-WF_CROPPED_ATLAS_MAP_BYTES);
 wf_grouped_crop_put(selection+24,memory_hash);wf_grouped_crop_put(selection+28,hint_hash);
 return wf_cropped_atlas_bind(out,selection,2048u,data,memory,hints,hint_bytes);
}
#endif
