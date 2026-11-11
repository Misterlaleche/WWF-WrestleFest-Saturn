#ifndef WF_TAG_SCENE_CACHE_H
#define WF_TAG_SCENE_CACHE_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#if defined(WF_COMBAT_TAG_SCENE6) && WF_COMBAT_TAG_SCENE6
#define WF_TAG_SCENE_COUNT 5u
#else
#define WF_TAG_SCENE_COUNT 4u
#endif

/* Immutable original scenes occupy only the checked tail of this match's
 * cart atlas. No scene bytes or startup scratch are retained in main RAM. */
typedef int (*wf_tag_scene_read_fn)(void*,const char*,uint32_t,void*,size_t);
typedef int (*wf_tag_scene_size_fn)(void*,const char*,uint32_t*);
typedef struct {
 uint8_t *cart;size_t capacity;
 struct {uint32_t offset,bytes;} records[WF_TAG_SCENE_COUNT];
 uint32_t atlas_bytes,end,required;bool active;
} wf_tag_scene_cache_t;
static inline const char*wf_tag_scene_name(unsigned i){
 switch(i){case 0:return "R18RING.BIN";case 1:return "R18MAP1.BIN";
 case 2:return "R18MAP2.BIN";case 3:return "R18MAP5.BIN";
#if defined(WF_COMBAT_TAG_SCENE6) && WF_COMBAT_TAG_SCENE6
 case 4:return "R18MAP6.BIN";
#endif
 default:return NULL;}
}
static inline uint32_t wf_tag_scene_bytes(unsigned i){
 switch(i){case 0:return 114918u;case 1:return 127138u;
 case 2:return 95978u;case 3:return 115568u;
#if defined(WF_COMBAT_TAG_SCENE6) && WF_COMBAT_TAG_SCENE6
 case 4:return 95198u;
#endif
 default:return 0;}
}
static inline int wf_tag_scene_index(const char*name){
 if(name)for(unsigned i=0;i<WF_TAG_SCENE_COUNT;i++)if(!strcmp(name,wf_tag_scene_name(i)))return (int)i;
 return -1;
}
static inline bool wf_ts_overlap(const void*a,size_t an,const void*b,size_t bn){
 uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;
 return an&&bn&&(x<=y?y-x<an:x-y<bn);
}
/* Failed/reopened caches revoke all prior spans. Preflight the complete tail
 * before reading any bytes; align each file to32 without assuming a3MiB base. */
static inline int wf_tag_scene_cache_open_masked(wf_tag_scene_cache_t*c,void*cart,size_t capacity,
 size_t atlas_memory,uint32_t required,wf_tag_scene_read_fn read,wf_tag_scene_size_fn size,void*user){
 if(!c)return -1;
 memset(c,0,sizeof *c);
 if(!required||(required&~((1u<<WF_TAG_SCENE_COUNT)-1u))||
    !cart||!read||!size||!capacity||capacity>0x400000u||atlas_memory>capacity||
    (uintptr_t)cart>UINTPTR_MAX-capacity||wf_ts_overlap(c,sizeof *c,cart,capacity))return -1;
 size_t end=atlas_memory;
 for(unsigned i=0;i<WF_TAG_SCENE_COUNT;i++){
  if(!(required&(1u<<i)))continue;
  uint32_t bytes;
  if(size(user,wf_tag_scene_name(i),&bytes)||bytes!=wf_tag_scene_bytes(i))return -2;
  end=(end+31u)&~(size_t)31u;
  if(end>capacity||bytes>capacity-end)return -1;
  end+=bytes;
 }
 if(((end+31u)&~(size_t)31u)>capacity)return -1;
 end=(atlas_memory+31u)&~(size_t)31u;
 for(unsigned i=0;i<WF_TAG_SCENE_COUNT;i++){
  if(!(required&(1u<<i)))continue;
  uint32_t bytes=wf_tag_scene_bytes(i);
  if(read(user,wf_tag_scene_name(i),0,(uint8_t*)cart+end,bytes)){
   memset(c,0,sizeof *c);return -3;
  }
  c->records[i].offset=(uint32_t)end;c->records[i].bytes=bytes;
  end=(end+bytes+31u)&~(size_t)31u;
 }
 c->cart=cart;c->capacity=capacity;c->atlas_bytes=(uint32_t)atlas_memory;c->end=(uint32_t)end;c->required=required;c->active=true;return 1;
}
/* The default API keeps every supported scene resident for older recipes. */
static inline int wf_tag_scene_cache_open(wf_tag_scene_cache_t*c,void*cart,size_t capacity,
 size_t atlas_memory,wf_tag_scene_read_fn read,wf_tag_scene_size_fn size,void*user){
 return wf_tag_scene_cache_open_masked(c,cart,capacity,atlas_memory,
   (1u<<WF_TAG_SCENE_COUNT)-1u,read,size,user);
}
static inline uint32_t wf_tag_scene_bit(unsigned scene){
 switch(scene){case 0:return 1u;case 1:return 2u;case 2:return 4u;case 5:return 8u;
#if defined(WF_COMBAT_TAG_SCENE6) && WF_COMBAT_TAG_SCENE6
 case 6:return 16u;
#endif
 default:return 0;}
}
/* Original $F98C chooses these inside/outside targets for each progression.
 * Include the actual initial scene as well; all later requests remain cache
 * only. Do not silently substitute a scene or fall back to combat CD reads. */
static inline int wf_tag_scene_cache_open_match(wf_tag_scene_cache_t*c,void*cart,size_t capacity,
 size_t atlas_memory,unsigned progression,unsigned initial,
 wf_tag_scene_read_fn read,wf_tag_scene_size_fn size,void*user){
 static const uint8_t outside[10]={2,6,0,6,2,6,2,2,6,2};
 static const uint8_t inside[10]={0,5,0,5,0,5,0,0,5,0};
 uint32_t mask=wf_tag_scene_bit(initial),a=0,b=0;
 if(progression<10u){a=wf_tag_scene_bit(outside[progression]);b=wf_tag_scene_bit(inside[progression]);}
 return wf_tag_scene_cache_open_masked(c,cart,capacity,atlas_memory,
   mask&&a&&b?(mask|a|b):0,read,size,user);
}
static inline bool wf_tag_scene_cache_valid(const wf_tag_scene_cache_t*c){
 if(!c||!c->active||!c->cart||!c->capacity||c->capacity>0x400000u||
    (uintptr_t)c->cart>UINTPTR_MAX-c->capacity||wf_ts_overlap(c,sizeof *c,c->cart,c->capacity))return false;
 if(!c->required||(c->required&~((1u<<WF_TAG_SCENE_COUNT)-1u))||c->atlas_bytes>c->capacity)return false;
 size_t end=(c->atlas_bytes+31u)&~(size_t)31u;
 for(unsigned i=0;i<WF_TAG_SCENE_COUNT;i++){
  if(!(c->required&(1u<<i))){
   if(c->records[i].offset||c->records[i].bytes)return false;
   continue;
  }
  if((end&31u)||c->records[i].offset!=end||c->records[i].bytes!=wf_tag_scene_bytes(i)||
     end>c->capacity||c->records[i].bytes>c->capacity-end)return false;
  end=(end+c->records[i].bytes+31u)&~(size_t)31u;
 }
 return end==c->end&&end<=c->capacity;
}
static int wf_tag_scene_cache_size(const wf_tag_scene_cache_t*c,const char*name,uint32_t*n){
 int i=wf_tag_scene_index(name);
 if(!n||i<0||!wf_tag_scene_cache_valid(c)||!(c->required&(1u<<i))||wf_ts_overlap(c,sizeof *c,n,sizeof *n)||
    wf_ts_overlap(c->cart,c->capacity,n,sizeof *n))return -1;
 *n=c->records[i].bytes;return 0;
}
static int wf_tag_scene_cache_read(const wf_tag_scene_cache_t*c,const char*name,uint32_t at,void*dst,size_t n){
 int i=wf_tag_scene_index(name);
 if(i<0||!wf_tag_scene_cache_valid(c)||!(c->required&(1u<<i))||(!dst&&n)||at>c->records[i].bytes||
    n>c->records[i].bytes-at||wf_ts_overlap(c,sizeof *c,dst,n)||
    wf_ts_overlap(c->cart,c->capacity,dst,n))return -1;
 if(n)memcpy(dst,c->cart+c->records[i].offset+at,n);
 return 0;
}
#endif
