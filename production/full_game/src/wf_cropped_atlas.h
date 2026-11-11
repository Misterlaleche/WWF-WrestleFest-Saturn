#ifndef WF_CROPPED_ATLAS_H
#define WF_CROPPED_ATLAS_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "wf_crop_copy.h"
#define WF_CROPPED_ATLAS_HEADER_BYTES 2048u
#define WF_CROPPED_ATLAS_MAP_BYTES 262144u
#define WF_CROPPED_ATLAS_HINT_BYTES 131072u
#define WF_CROPPED_ATLAS_PACKED 0x8000u
typedef struct {const uint8_t *data,*hints;size_t bytes;unsigned count;} wf_cropped_atlas_t;
static inline uint32_t wf_cropped_atlas_word(const uint8_t*p){
 return (uint32_t)p[0]<<24|(uint32_t)p[1]<<16|(uint32_t)p[2]<<8|p[3];
}
static inline uint32_t wf_cropped_atlas_hash(const uint8_t*p,size_t n){
 uint32_t h=2166136261u;while(n--)h=(h^*p++)*16777619u;return h;
}
static inline bool wf_cropped_atlas_shape(uint16_t crop){
 unsigned meta=crop>>8,top=meta&31u,height=crop&255u;
 return !(meta&128u)&&top<=16u&&height<=16u&&top+height<=16u&&
        (!(meta&64u)||(meta&32u));
}
static inline bool wf_cropped_atlas_extent(const uint8_t*h,size_t n,size_t capacity,size_t *bytes){
 if(bytes)*bytes=0;
 if(!h||!bytes||n<WF_CROPPED_ATLAS_HEADER_BYTES||memcmp(h,"R19C",4)||
    wf_cropped_atlas_word(h+4)!=WF_CROPPED_ATLAS_HEADER_BYTES||
    wf_cropped_atlas_word(h+12)!=65536u)return false;
 uint32_t total=wf_cropped_atlas_word(h+8),count=wf_cropped_atlas_word(h+16),pixels=wf_cropped_atlas_word(h+20);
 if(!count||count>65536u||capacity<WF_CROPPED_ATLAS_MAP_BYTES||
    pixels>capacity-WF_CROPPED_ATLAS_MAP_BYTES||
    pixels>0x400000u-WF_CROPPED_ATLAS_MAP_BYTES||
    total!=WF_CROPPED_ATLAS_MAP_BYTES+pixels||(pixels&3u))return false;
 for(unsigned i=32;i<WF_CROPPED_ATLAS_HEADER_BYTES;i++)if(h[i])return false;
 *bytes=total;return true;
}
static inline bool wf_cropped_atlas_bind(wf_cropped_atlas_t*a,const uint8_t*h,size_t n,
 const uint8_t*data,size_t bytes,const uint8_t*hints,size_t hint_bytes){
 if(!a)return false;
 *a=(wf_cropped_atlas_t){0};size_t expected;
 if(!data||!hints||hint_bytes!=WF_CROPPED_ATLAS_HINT_BYTES||
    !wf_cropped_atlas_extent(h,n,bytes,&expected)||bytes!=expected||
    wf_cropped_atlas_hash(data,bytes)!=wf_cropped_atlas_word(h+24)||
    wf_cropped_atlas_hash(hints,hint_bytes)!=wf_cropped_atlas_word(h+28))return false;
 size_t cursor=WF_CROPPED_ATLAS_MAP_BYTES;unsigned count=0;
 for(unsigned i=0;i<65536u;i++){
  uint16_t crop=(uint16_t)((unsigned)hints[i*2u]<<8|hints[i*2u+1u]);
  if(!wf_cropped_atlas_shape(crop))return false;
  uint32_t entry=wf_cropped_atlas_word(data+i*4u);if(!entry)continue;
  unsigned length=(crop&255u)*((crop&0x2000u)?4u:8u);
  if(entry-1u!=cursor||length>bytes-cursor)return false;
  cursor+=length;count++;
 }
 if(cursor!=bytes||count!=wf_cropped_atlas_word(h+16))return false;
 *a=(wf_cropped_atlas_t){.data=data,.hints=hints,.bytes=bytes,.count=count};return true;
}
static inline bool wf_cropped_atlas_get(const wf_cropped_atlas_t*a,uint16_t tile,const uint8_t**out,uint16_t*crop){
 if(out)*out=NULL;
 if(crop)*crop=0xffffu;
 if(!a||!a->data||!a->hints||!out||!crop)return false;
 uint32_t entry=wf_cropped_atlas_word(a->data+(unsigned)tile*4u);if(!entry)return false;
 *out=a->data+(entry-1u);
 *crop=(uint16_t)((unsigned)a->hints[(unsigned)tile*2u]<<8|a->hints[(unsigned)tile*2u+1u]|WF_CROPPED_ATLAS_PACKED);
 return true;
}
/* Source contains only visible rows, already reduced to4bytes when narrow.
 * Preserve the existing VDP1 staging layout; caller strips the packed flag. */
static inline bool wf_crop_copy_packed(uint8_t*dst,const uint8_t*src,uint16_t crop){
 if(!dst||!src||!wf_cropped_atlas_shape(crop))return false;
 unsigned top=(crop>>8)&31u,height=crop&255u;
 unsigned first=(crop&0x2000u)?0u:top*8u;
 unsigned bytes=height*((crop&0x2000u)?4u:8u);
 memset(dst,0,first);wf_crop_copy_words(dst+first,src,bytes);
 memset(dst+first+bytes,0,128u-first-bytes);return true;
}
#endif
