#ifndef WF_PROFILE_ARCHIVE_H
#define WF_PROFILE_ARCHIVE_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

/* Startup-only, allocation-free Royal archive. Selected payloads retain their
 * original R22S/R18W validation. The atlas loader validates pool membership and
 * memory separately before gameplay can begin. */
enum { WF_PROFILE_ARCHIVE_SCRATCH=8192u, WF_PROFILE_ARCHIVE_WARM=8u+2u*3584u,
       WF_PROFILE_ARCHIVE_DATA=2048u+8192u*32u };
typedef int (*wf_profile_archive_read_fn)(void*,uint32_t,void*,size_t);
typedef struct {
 wf_profile_archive_read_fn read;void *user;
 uint32_t selector_offset,selector_bytes,warm_bytes;
 bool active;
 uint8_t warm[WF_PROFILE_ARCHIVE_WARM];
} wf_profile_archive_t;
static inline uint32_t wf_pa_word(const uint8_t*p){return (uint32_t)p[0]<<24|(uint32_t)p[1]<<16|(uint32_t)p[2]<<8|p[3];}
static inline uint32_t wf_pa_hash(const uint8_t*p,size_t n){uint32_t h=2166136261u;while(n--)h=(h^*p++)*16777619u;return h;}
static inline bool wf_pa_zero(const uint8_t*p,size_t n){while(n--)if(*p++)return false;return true;}
static inline bool wf_pa_range(uint32_t at,uint32_t n,uint32_t total){return at>=WF_PROFILE_ARCHIVE_DATA&&at<=total&&n<=total-at;}

/* Returns 1 found, 0 absent, or a negative reason. No active binding survives
 * a failed/absent open. The caller supplies disjoint context/scratch storage. */
static int wf_profile_archive_open(wf_profile_archive_t*a,wf_profile_archive_read_fn read,
 void*user,uint32_t total,uint32_t key,void*scratch,size_t capacity){
 if(!a)return -1;
 memset(a,0,sizeof *a);
 if(!read||!scratch||capacity<WF_PROFILE_ARCHIVE_SCRATCH||key<=0x1000u||key>=0x2000u||total<WF_PROFILE_ARCHIVE_DATA||total&2047u)return -1;
 uint8_t*h=scratch;
 if(read(user,0,h,2048))return -2;
 if(memcmp(h,"R23A",4)||wf_pa_word(h+4)!=1u||wf_pa_word(h+8)!=2048u||
    wf_pa_word(h+12)!=8192u||wf_pa_word(h+16)!=32u||wf_pa_word(h+20)!=2048u||
    wf_pa_word(h+24)!=WF_PROFILE_ARCHIVE_DATA||wf_pa_word(h+28)!=total||
    !wf_pa_word(h+32)||wf_pa_word(h+32)>400u||!wf_pa_zero(h+36,2008)||
    wf_pa_hash(h,2044)!=wf_pa_word(h+2044))return -3;
 uint8_t entry[32];
 if(read(user,2048u+key*32u,entry,sizeof entry))return -2;
 if(wf_pa_zero(entry,sizeof entry))return 0;
 uint32_t at=wf_pa_word(entry+4),bytes=wf_pa_word(entry+8),wa=wf_pa_word(entry+12),wn=wf_pa_word(entry+16);
 if(wf_pa_word(entry)!=key||bytes!=10240u||(at&2047u)||(wa&2047u)||
    !wf_pa_range(at,bytes,total)||!wf_pa_range(wa,wn,total)||wa<at+bytes||
    wn<10u||wn>sizeof a->warm||wf_pa_word(entry+24)||
    wf_pa_hash(entry,28)!=wf_pa_word(entry+28))return -4;
 if(read(user,at,h,2048))return -2;
 if(memcmp(h,"R22S",4)||wf_pa_word(h+4)!=1u||wf_pa_word(h+8)!=2048u||
    wf_pa_word(h+12)!=(key&4095u)||wf_pa_word(h+16)!=1u||
    wf_pa_hash(h,2044)!=wf_pa_word(h+2044))return -5;
 uint32_t bitmap_hash=wf_pa_word(h+40);
 if(read(user,wa,a->warm,wn)||read(user,at+2048u,h,8192))return -2;
 if(wf_pa_hash(a->warm,wn)!=wf_pa_word(entry+20)||wf_pa_hash(h,8192)!=bitmap_hash||
    memcmp(a->warm,"R18W",4)||a->warm[4]||a->warm[5]!=1u)return -6;
 unsigned count=(unsigned)a->warm[6]<<8|a->warm[7];
 if(!count||wn!=8u+2u*count)return -6;
 for(unsigned i=0;i<count;i++){
  unsigned tile=(unsigned)a->warm[8u+2u*i]<<8|a->warm[9u+2u*i],bit=1u<<(tile&7u);
  if(!(h[tile>>3]&bit))return -6;
  h[tile>>3]&=(uint8_t)~bit;
 }
 a->read=read;a->user=user;a->selector_offset=at;a->selector_bytes=bytes;a->warm_bytes=wn;a->active=true;
 return 1;
}
/* kind0 is the on-disc selector; kind1 the already validated warm list. */
static int wf_profile_archive_read(const wf_profile_archive_t*a,unsigned kind,uint32_t at,void*dst,size_t bytes){
 if(!a||!a->active||!a->read||kind>1u||(!dst&&bytes))return -1;
 uint32_t length=kind?a->warm_bytes:a->selector_bytes;
 if(at>length||bytes>length-at)return -1;
 if(!bytes)return 0;
 if(kind){memcpy(dst,a->warm+at,bytes);return 0;}
 return a->read(a->user,a->selector_offset+at,dst,bytes);
}
#endif
