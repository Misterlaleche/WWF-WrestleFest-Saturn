#ifndef WF_GLOBAL_RANS_H
#define WF_GLOBAL_RANS_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include "wp2015_secondary.h"
#ifdef WF_GLOBAL_RANS_PROFILE
#ifdef WF_GLOBAL_RANS_PROFILE_TEST
unsigned wf_global_rans_test_ticks(void);
#define WF_GR_NOW() wf_global_rans_test_ticks()
#else
#include <yaul.h>
#define WF_GR_NOW() cpu_frt_count_get()
#endif
extern volatile uint32_t wf_global_rans_cost[8];
#define WF_GR_BEGIN() unsigned wf_gr_start=WF_GR_NOW(),wf_gr_stamp=wf_gr_start
#define WF_GR_MARK(slot) do{unsigned wf_gr_now=WF_GR_NOW();wf_global_rans_cost[slot]+=(uint16_t)(wf_gr_now-wf_gr_stamp);wf_gr_stamp=wf_gr_now;}while(0)
#define WF_GR_END() do{wf_global_rans_cost[0]+=(uint16_t)(WF_GR_NOW()-wf_gr_start);}while(0)
#else
#define WF_GR_BEGIN() do{}while(0)
#define WF_GR_MARK(slot) do{}while(0)
#define WF_GR_END() do{}while(0)
#endif
#ifdef WF_GLOBAL_RANS_TRACE
enum { WF_GLOBAL_RANS_TRACE_CAPACITY=16384 };
extern volatile uint16_t wf_global_rans_trace_tiles[WF_GLOBAL_RANS_TRACE_CAPACITY];
extern volatile uint32_t wf_global_rans_trace_count,wf_global_rans_trace_overflow;
static inline void wf_global_rans_trace_append(uint16_t tile){
 uint32_t count=wf_global_rans_trace_count;
 if(count<WF_GLOBAL_RANS_TRACE_CAPACITY){
  wf_global_rans_trace_tiles[count]=tile;wf_global_rans_trace_count=count+1u;
 }else wf_global_rans_trace_overflow=1;
}
#endif

/* Complete immutable sprite corpus, using the unchanged TAG rANS decoder. */
#ifdef WF_GLOBAL_RANS_SMALL_BLOCKS
enum { WF_GLOBAL_RANS_HEADER=2048, WF_GLOBAL_RANS_BLOCK=512,
 WF_GLOBAL_RANS_BLOCKS=16384,
#ifdef WF_GLOBAL_RANS_PRIORITY
 WF_GLOBAL_RANS_CACHE=96, WF_GLOBAL_RANS_CART_SETS=64,
#else
 WF_GLOBAL_RANS_CACHE=104, WF_GLOBAL_RANS_CART_SETS=128,
#endif
 WF_GLOBAL_RANS_CART_BLOCKS=1024,
 WF_GLOBAL_RANS_DIRECTORY=2048, WF_GLOBAL_RANS_MODEL=198656,
 WF_GLOBAL_RANS_PAYLOAD=200704, WF_GLOBAL_RANS_GUARD_WORD=0x6a29c581u,
 WF_GLOBAL_RANS_VERSION=2, WF_GLOBAL_RANS_TILE_SHIFT=2 };
#else
enum { WF_GLOBAL_RANS_HEADER=2048, WF_GLOBAL_RANS_BLOCK=1024,
 WF_GLOBAL_RANS_BLOCKS=8192,
#ifdef WF_GLOBAL_RANS_PRIORITY
 WF_GLOBAL_RANS_CACHE=60, WF_GLOBAL_RANS_CART_SETS=64,
#else
 WF_GLOBAL_RANS_CACHE=64, WF_GLOBAL_RANS_CART_SETS=128,
#endif
 WF_GLOBAL_RANS_CART_BLOCKS=512,
 WF_GLOBAL_RANS_DIRECTORY=2048, WF_GLOBAL_RANS_MODEL=100352,
 WF_GLOBAL_RANS_PAYLOAD=102400, WF_GLOBAL_RANS_GUARD_WORD=0x6a29c581u,
 WF_GLOBAL_RANS_VERSION=1, WF_GLOBAL_RANS_TILE_SHIFT=3 };
#endif
enum { WF_GLOBAL_RANS_TILES=WF_GLOBAL_RANS_BLOCK/128,
 WF_GLOBAL_RANS_CART_WAYS=WF_GLOBAL_RANS_CART_BLOCKS/WF_GLOBAL_RANS_CART_SETS };
_Static_assert(WF_GLOBAL_RANS_BLOCKS*WF_GLOBAL_RANS_BLOCK==65536u*128u,"Complete original corpus");
_Static_assert(WF_GLOBAL_RANS_CACHE<255u,"Byte cache map slot bounds");
enum { WF_GLOBAL_RANS_ARGUMENT=-1, WF_GLOBAL_RANS_IO=-2,
 WF_GLOBAL_RANS_HEADER_ERROR=-3, WF_GLOBAL_RANS_METADATA=-4,
 WF_GLOBAL_RANS_SOURCE=-5, WF_GLOBAL_RANS_MODEL_ERROR=-6,
 WF_GLOBAL_RANS_DECODE=-7, WF_GLOBAL_RANS_PIXELS=-8, WF_GLOBAL_RANS_GUARD=-9 };
typedef struct {
 uint32_t guard[8];
 wp2015_rans_compact_t tables;
 uint32_t tags[WF_GLOBAL_RANS_CACHE];
 uint8_t map[WF_GLOBAL_RANS_BLOCKS];
 uint32_t age[WF_GLOBAL_RANS_CACHE];
 uint16_t cart_tags[WF_GLOBAL_RANS_CART_BLOCKS];
 uint32_t cart_age[WF_GLOBAL_RANS_CART_BLOCKS];
#ifdef WF_GLOBAL_RANS_PRIORITY
 uint16_t cart_frequency[WF_GLOBAL_RANS_CART_BLOCKS];
 uint16_t cart_encoded[WF_GLOBAL_RANS_CART_BLOCKS];
#endif
 uint8_t cache[WF_GLOBAL_RANS_CACHE][WF_GLOBAL_RANS_BLOCK];
 uint8_t input[WF_GLOBAL_RANS_BLOCK];
 uint32_t end_guard[8];
} wf_global_rans_workspace_t;
_Static_assert(sizeof(wp2015_rans_compact_t)==2620u,"Original TAG decoder table geometry");
typedef struct {
 const uint8_t *data;
 uint8_t *cart_cache;
 wf_global_rans_workspace_t *workspace;
 uint32_t bytes,payload_bytes,hits,misses,last_tile,selected,cart_hits,clock;
 int error;
} wf_global_rans_t;
typedef int (*wf_global_rans_read_fn)(void *,uint32_t,void *,size_t);
int wp2015_rans_tables(uint32_t *,unsigned,const uint16_t[286],const uint16_t[30]);
int wp2015_rans_decode(void *,unsigned *,const void *,unsigned,const uint32_t *);

static inline uint32_t wf_global_rans_word(const uint8_t*p){
 return (uint32_t)p[0]<<24|(uint32_t)p[1]<<16|(uint32_t)p[2]<<8|p[3];
}
#ifdef WF_GLOBAL_RANS_FAST_HASH
static inline uint32_t wf_global_rans_hash_update(uint32_t h,const uint8_t*p,size_t n){
#if defined(__sh__)
 /* Four bounded byte reads preserve FNV order and unaligned input support.
  * GCC's byte postincrement otherwise adds redundant address calculations.
  * Only complete groups enter assembly; the C tail reads exactly n%4 bytes. */
 size_t groups=n>>2;n&=3u;
 if(groups){
  const uint32_t prime=16777619u;
  __asm__ volatile (
   "1:\n\tmov.b @%1+,r0\n\textu.b r0,r0\n\txor r0,%0\n\tmul.l %3,%0\n\tsts macl,%0\n"
   "mov.b @%1+,r0\n\textu.b r0,r0\n\txor r0,%0\n\tmul.l %3,%0\n\tsts macl,%0\n"
   "mov.b @%1+,r0\n\textu.b r0,r0\n\txor r0,%0\n\tmul.l %3,%0\n\tsts macl,%0\n"
   "mov.b @%1+,r0\n\textu.b r0,r0\n\txor r0,%0\n\tmul.l %3,%0\n\tdt %2\n\tbf/s 1b\n\tsts macl,%0"
   : "+&r"(h), "+&r"(p), "+&r"(groups) : "r"(prime) : "r0", "macl", "t", "memory");
 }
#else
 while(n>=4u){
  h=(h^p[0])*16777619u;h=(h^p[1])*16777619u;
  h=(h^p[2])*16777619u;h=(h^p[3])*16777619u;p+=4;n-=4;
 }
#endif
 while(n--)h=(h^*p++)*16777619u;
 return h;
}
#endif
static inline uint32_t wf_global_rans_hash(const uint8_t*p,size_t n){
#ifdef WF_GLOBAL_RANS_FAST_HASH
 return wf_global_rans_hash_update(2166136261u,p,n);
#else
 uint32_t h=2166136261u;while(n--)h=(h^*p++)*16777619u;return h;
#endif
}
static inline bool wf_global_rans_zero(const uint8_t*p,size_t n){
 while(n--){if(*p++)return false;}
 return true;
}
static inline uintptr_t wf_global_rans_physical(const void*p){
 uintptr_t n=(uintptr_t)p;
#if defined(__sh__) || defined(WF_PLATFORM_SATURN)
 n&=0x1fffffffu;
#endif
 return n;
}
static inline bool wf_global_rans_disjoint(const void*a,size_t an,const void*b,size_t bn){
 uintptr_t x=wf_global_rans_physical(a),y=wf_global_rans_physical(b);
 if(an>UINTPTR_MAX-x||bn>UINTPTR_MAX-y)return false;
#if defined(__sh__) || defined(WF_PLATFORM_SATURN)
 if(an>0x20000000u-x||bn>0x20000000u-y)return false;
#endif
 return x+an<=y||y+bn<=x;
}
static inline int wf_global_rans_fail(wf_global_rans_t*c,int error){
 c->data=NULL;c->cart_cache=NULL;c->workspace=NULL;c->error=error;return error;
}
static inline uint32_t wf_global_rans_fast_hash(const uint8_t*p,size_t n,wf_global_rans_workspace_t*w){
 /* The cache is not live during bind. Hash actual cartridge bytes in HWRAM. */
 uint32_t h=2166136261u;uint8_t*scratch=(uint8_t*)w->cache;
 while(n){size_t take=n>16384u?16384u:n;memcpy(scratch,p,take);
#ifdef WF_GLOBAL_RANS_FAST_HASH
  h=wf_global_rans_hash_update(h,scratch,take);
#else
  for(size_t i=0;i<take;i++)h=(h^scratch[i])*16777619u;
#endif
  p+=take;n-=take;
 }return h;
}
static inline bool wf_global_rans_header(const uint8_t*p,uint32_t bytes){
 return bytes>=WF_GLOBAL_RANS_PAYLOAD&&!(bytes&2047u)&&
  !memcmp(p,"R24G",4)&&wf_global_rans_word(p+4)==WF_GLOBAL_RANS_VERSION&&
  wf_global_rans_word(p+8)==2048u&&wf_global_rans_word(p+12)==65536u&&
  wf_global_rans_word(p+16)==128u&&wf_global_rans_word(p+20)==WF_GLOBAL_RANS_BLOCK&&
  wf_global_rans_word(p+24)==WF_GLOBAL_RANS_BLOCKS&&wf_global_rans_word(p+28)==2048u&&
  wf_global_rans_word(p+32)==WF_GLOBAL_RANS_BLOCKS*12u&&wf_global_rans_word(p+36)==WF_GLOBAL_RANS_MODEL&&
  wf_global_rans_word(p+40)==632u&&wf_global_rans_word(p+44)==WF_GLOBAL_RANS_PAYLOAD&&
  wf_global_rans_word(p+52)==bytes&&wf_global_rans_word(p+68)==1u&&
  wf_global_rans_word(p+72)==WF_GLOBAL_RANS_BLOCK&&wf_global_rans_zero(p+76,2044u-76u)&&
  wf_global_rans_hash(p,2044u)==wf_global_rans_word(p+2044u);
}
static inline int wf_global_rans_load(wf_global_rans_t*c,void*cart,size_t capacity,
 uint32_t bytes,wf_global_rans_read_fn read,void*user,void*workspace,size_t workspace_bytes){
 if(!c)return WF_GLOBAL_RANS_ARGUMENT;
 /* Reject aliasing before clearing a context which might alias its source. */
 if(!wf_global_rans_disjoint(c,sizeof*c,cart,capacity)||
    !wf_global_rans_disjoint(c,sizeof*c,workspace,workspace_bytes))return WF_GLOBAL_RANS_ARGUMENT;
 memset(c,0,sizeof*c);
 if(!cart||!workspace||!read||((uintptr_t)cart&31u)||((uintptr_t)workspace&31u)||
    capacity>0x400000u||capacity<WF_GLOBAL_RANS_CART_BLOCKS*WF_GLOBAL_RANS_BLOCK||
    bytes>capacity-WF_GLOBAL_RANS_CART_BLOCKS*WF_GLOBAL_RANS_BLOCK||bytes<WF_GLOBAL_RANS_PAYLOAD||
    workspace_bytes<sizeof(wf_global_rans_workspace_t)||
    !wf_global_rans_disjoint(cart,capacity,workspace,workspace_bytes))return wf_global_rans_fail(c,WF_GLOBAL_RANS_ARGUMENT);
 uint8_t*p=cart;wf_global_rans_workspace_t*w=workspace;
 c->selected=1;
 if(read(user,0,p,WF_GLOBAL_RANS_HEADER))return wf_global_rans_fail(c,WF_GLOBAL_RANS_IO);
 if(!wf_global_rans_header(p,bytes))return wf_global_rans_fail(c,WF_GLOBAL_RANS_HEADER_ERROR);
 uint32_t payload=wf_global_rans_word(p+48);
 if(!payload||payload>bytes-WF_GLOBAL_RANS_PAYLOAD||
    bytes-WF_GLOBAL_RANS_PAYLOAD-payload>=2048u)return wf_global_rans_fail(c,WF_GLOBAL_RANS_HEADER_ERROR);
 if(read(user,WF_GLOBAL_RANS_HEADER,p+WF_GLOBAL_RANS_HEADER,bytes-WF_GLOBAL_RANS_HEADER))return wf_global_rans_fail(c,WF_GLOBAL_RANS_IO);
 if(wf_global_rans_fast_hash(p+2048,WF_GLOBAL_RANS_PAYLOAD-2048,w)!=wf_global_rans_word(p+56)||
    !wf_global_rans_zero(p+WF_GLOBAL_RANS_MODEL+632,WF_GLOBAL_RANS_PAYLOAD-WF_GLOBAL_RANS_MODEL-632))return wf_global_rans_fail(c,WF_GLOBAL_RANS_METADATA);
 if(wf_global_rans_fast_hash(p+WF_GLOBAL_RANS_PAYLOAD,payload,w)!=wf_global_rans_word(p+60)||
    !wf_global_rans_zero(p+WF_GLOBAL_RANS_PAYLOAD+payload,bytes-WF_GLOBAL_RANS_PAYLOAD-payload))return wf_global_rans_fail(c,WF_GLOBAL_RANS_SOURCE);
 uint32_t end=0;
 for(unsigned i=0;i<WF_GLOBAL_RANS_BLOCKS;i++){
  const uint8_t*entry=p+WF_GLOBAL_RANS_DIRECTORY+i*12u;
  uint32_t at=wf_global_rans_word(entry),n=wf_global_rans_word(entry+4);
  if(at!=end||(at&3u)||n<4u||n>WF_GLOBAL_RANS_BLOCK||at>payload||n>payload-at)return wf_global_rans_fail(c,WF_GLOBAL_RANS_METADATA);
  uint32_t rounded=(n+3u)&~3u;
  if(rounded>payload-at||!wf_global_rans_zero(p+WF_GLOBAL_RANS_PAYLOAD+at+n,rounded-n))return wf_global_rans_fail(c,WF_GLOBAL_RANS_METADATA);
  end=at+rounded;
 }
 if(end!=payload)return wf_global_rans_fail(c,WF_GLOBAL_RANS_METADATA);
 uint16_t frequencies[316];
 for(unsigned i=0;i<316u;i++)frequencies[i]=(uint16_t)((unsigned)p[WF_GLOBAL_RANS_MODEL+i*2u]<<8|p[WF_GLOBAL_RANS_MODEL+i*2u+1u]);
 if(wp2015_rans_tables((uint32_t*)&w->tables,sizeof w->tables/4u,frequencies,frequencies+286))return wf_global_rans_fail(c,WF_GLOBAL_RANS_MODEL_ERROR);
 for(unsigned i=0;i<WF_GLOBAL_RANS_CACHE;i++)w->tags[i]=UINT32_MAX;
 memset(w->map,0,sizeof w->map);memset(w->age,0,sizeof w->age);
 memset(w->cart_age,0,sizeof w->cart_age);
#ifdef WF_GLOBAL_RANS_PRIORITY
 memset(w->cart_frequency,0,sizeof w->cart_frequency);
 memset(w->cart_encoded,0,sizeof w->cart_encoded);
#endif
 for(unsigned i=0;i<WF_GLOBAL_RANS_CART_BLOCKS;i++)w->cart_tags[i]=UINT16_MAX;
 for(unsigned i=0;i<8u;i++)w->guard[i]=w->end_guard[i]=WF_GLOBAL_RANS_GUARD_WORD;
 c->bytes=bytes;c->payload_bytes=payload;c->workspace=w;c->cart_cache=p+bytes;c->data=p;return 0;
}
static inline int wf_global_rans_get(wf_global_rans_t*c,uint16_t tile,const uint8_t**out){
 if(out)*out=NULL;
 if(!c||!out)return WF_GLOBAL_RANS_ARGUMENT;
 c->last_tile=tile;
 if(!c->data||!c->cart_cache||!c->workspace)return wf_global_rans_fail(c,WF_GLOBAL_RANS_ARGUMENT);
#ifdef WF_GLOBAL_RANS_TRACE
 /* Observation only, outside the fine provider timer; never wrap the buffer. */
 wf_global_rans_trace_append(tile);
#endif
 WF_GR_BEGIN();
 wf_global_rans_workspace_t*w=c->workspace;
 for(unsigned i=0;i<8u;i++)if(w->guard[i]!=WF_GLOBAL_RANS_GUARD_WORD||w->end_guard[i]!=WF_GLOBAL_RANS_GUARD_WORD)return wf_global_rans_fail(c,WF_GLOBAL_RANS_GUARD);
 if(!++c->clock){memset(w->age,0,sizeof w->age);memset(w->cart_age,0,sizeof w->cart_age);c->clock=1;}
 unsigned block=tile>>WF_GLOBAL_RANS_TILE_SHIFT,slot=w->map[block];
 WF_GR_MARK(1);
 if(slot){
  slot--;
  if(slot>=WF_GLOBAL_RANS_CACHE||w->tags[slot]!=block)return wf_global_rans_fail(c,WF_GLOBAL_RANS_GUARD);
  c->hits++;
 }else{
  slot=0;
  for(unsigned i=0;i<WF_GLOBAL_RANS_CACHE;i++){
   if(w->tags[i]==UINT32_MAX){slot=i;break;}
   if(w->age[i]<w->age[slot])slot=i;
  }
  if(w->tags[slot]!=UINT32_MAX){
   if(w->tags[slot]>=WF_GLOBAL_RANS_BLOCKS)return wf_global_rans_fail(c,WF_GLOBAL_RANS_GUARD);
   w->map[w->tags[slot]]=0;
  }
  w->tags[slot]=UINT32_MAX;
  const uint8_t*entry=c->data+WF_GLOBAL_RANS_DIRECTORY+block*12u;
  uint32_t at=wf_global_rans_word(entry),n=wf_global_rans_word(entry+4);
  if((at&3u)||n<4u||n>WF_GLOBAL_RANS_BLOCK||at>c->payload_bytes||n>c->payload_bytes-at)return wf_global_rans_fail(c,WF_GLOBAL_RANS_METADATA);
  unsigned first=(block&(WF_GLOBAL_RANS_CART_SETS-1u))*WF_GLOBAL_RANS_CART_WAYS,cart_slot=first;
  bool found=false;unsigned empty=WF_GLOBAL_RANS_CART_BLOCKS;
#ifdef WF_GLOBAL_RANS_PRIORITY
  uint32_t minimum_score=UINT32_MAX;
  for(unsigned i=first;i<first+WF_GLOBAL_RANS_CART_WAYS;i++){
   if(w->cart_tags[i]==UINT16_MAX){empty=i;continue;}
   if(w->cart_tags[i]>=WF_GLOBAL_RANS_BLOCKS||!w->cart_frequency[i]||
      w->cart_encoded[i]<4u||w->cart_encoded[i]>WF_GLOBAL_RANS_BLOCK)return wf_global_rans_fail(c,WF_GLOBAL_RANS_GUARD);
   if(w->cart_tags[i]==block){
    if(w->cart_encoded[i]!=n)return wf_global_rans_fail(c,WF_GLOBAL_RANS_GUARD);
    cart_slot=i;found=true;break;
   }
   /* At most65535*(1024+16), so the product cannot wrap uint32. */
   uint32_t score=(uint32_t)w->cart_frequency[i]*(w->cart_encoded[i]+16u);
   if(score<minimum_score||(score==minimum_score&&w->cart_age[i]<w->cart_age[cart_slot])){
    minimum_score=score;cart_slot=i;
   }
  }
#else
  for(unsigned i=first;i<first+WF_GLOBAL_RANS_CART_WAYS;i++){
   if(w->cart_tags[i]==block){cart_slot=i;found=true;break;}
   if(w->cart_tags[i]==UINT16_MAX)empty=i;
   if(w->cart_age[i]<w->cart_age[cart_slot])cart_slot=i;
  }
#endif
  WF_GR_MARK(2);
  if(found){
   memcpy(w->cache[slot],c->cart_cache+cart_slot*WF_GLOBAL_RANS_BLOCK,WF_GLOBAL_RANS_BLOCK);c->cart_hits++;
   WF_GR_MARK(6);
  }else{
   if(empty!=WF_GLOBAL_RANS_CART_BLOCKS)cart_slot=empty;
   w->cart_tags[cart_slot]=UINT16_MAX;
   memcpy(w->input,c->data+WF_GLOBAL_RANS_PAYLOAD+at,n);
   WF_GR_MARK(3);
   unsigned output_bytes=WF_GLOBAL_RANS_BLOCK;
   if(wp2015_rans_decode(w->cache[slot],&output_bytes,w->input,n,(const uint32_t*)&w->tables)||output_bytes!=WF_GLOBAL_RANS_BLOCK)return wf_global_rans_fail(c,WF_GLOBAL_RANS_DECODE);
   WF_GR_MARK(4);
  }
  if(wf_global_rans_hash(w->cache[slot],WF_GLOBAL_RANS_BLOCK)!=wf_global_rans_word(entry+8))return wf_global_rans_fail(c,WF_GLOBAL_RANS_PIXELS);
  WF_GR_MARK(5);
  if(!found){
   memcpy(c->cart_cache+cart_slot*WF_GLOBAL_RANS_BLOCK,w->cache[slot],WF_GLOBAL_RANS_BLOCK);
   WF_GR_MARK(6);
#ifdef WF_GLOBAL_RANS_PRIORITY
   w->cart_frequency[cart_slot]=1;w->cart_encoded[cart_slot]=(uint16_t)n;
#endif
   w->cart_tags[cart_slot]=(uint16_t)block;c->misses++;
  }
#ifdef WF_GLOBAL_RANS_PRIORITY
  else if(w->cart_frequency[cart_slot]!=UINT16_MAX)w->cart_frequency[cart_slot]++;
#endif
  w->cart_age[cart_slot]=c->clock;w->tags[slot]=block;w->map[block]=(uint8_t)(slot+1u);
 }
 w->age[slot]=c->clock;
 *out=w->cache[slot]+(tile&(WF_GLOBAL_RANS_TILES-1u))*128u;
 WF_GR_MARK(7);WF_GR_END();return 0;
}
#undef WF_GR_BEGIN
#undef WF_GR_MARK
#undef WF_GR_END
#ifdef WF_GR_NOW
#undef WF_GR_NOW
#endif
#endif
