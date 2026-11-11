#ifndef WF_TAG_ATLAS_ARCHIVE_H
#define WF_TAG_ATLAS_ARCHIVE_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include "wf_cropped_atlas.h"

/* Startup-only WTA1 slice. The original R19C provider validates the full map,
 * pixels and crop hints after this bounded three-read metadata check. */
enum {WF_TAG_ATLAS_ARCHIVE_SCRATCH=2048u, WF_TAG_ATLAS_ARCHIVE_DATA=133120u};
typedef int (*wf_tag_atlas_archive_read_fn)(void*,uint32_t,void*,size_t);
typedef struct {
 wf_tag_atlas_archive_read_fn read;void *user;
 uint32_t offset,bytes,key;bool active;
} wf_tag_atlas_archive_t;
static inline bool wf_ta_zero(const uint8_t*p,size_t n){while(n--)if(*p++)return false;return true;}
static inline bool wf_ta_overlap(const void*a,size_t an,const void*b,size_t bn){
 uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;
 return an&&bn&&(x<=y?y-x<an:x-y<bn);
}
static inline bool wf_ta_key(uint32_t key){
 unsigned count=0;if(!key||key>=4096u)return false;
 while(key){key&=key-1u;count++;}return count==4u;
}
/* Returns 1 found, 0 absent, negative malformed/I/O. All failed opens revoke
 * the previous binding. Scratch's used 2048-byte span must be disjoint from
 * the context; it is never retained. cart_capacity is a separate actual budget.
 * Whole-index count/overlap validation belongs to host packaging, not startup. */
static int wf_tag_atlas_archive_open(wf_tag_atlas_archive_t*a,wf_tag_atlas_archive_read_fn read,
 void*user,uint32_t total,uint32_t key,void*scratch,size_t scratch_bytes,size_t cart_capacity){
 if(!a)return -1;
 memset(a,0,sizeof *a);
 if(!read||!scratch||scratch_bytes<WF_TAG_ATLAS_ARCHIVE_SCRATCH||
    wf_ta_overlap(a,sizeof *a,scratch,WF_TAG_ATLAS_ARCHIVE_SCRATCH)||
    cart_capacity<WF_CROPPED_ATLAS_MAP_BYTES||cart_capacity>0x400000u||
    !wf_ta_key(key)||total<WF_TAG_ATLAS_ARCHIVE_DATA||(total&2047u))return -1;
 uint8_t*h=scratch;
 if(read(user,0,h,2048))return -2;
 if(memcmp(h,"WTA1",4)||wf_cropped_atlas_word(h+4)!=1u||
    wf_cropped_atlas_word(h+8)!=2048u||wf_cropped_atlas_word(h+12)!=4096u||
    wf_cropped_atlas_word(h+16)!=32u||wf_cropped_atlas_word(h+20)!=2048u||
    wf_cropped_atlas_word(h+24)!=WF_TAG_ATLAS_ARCHIVE_DATA||wf_cropped_atlas_word(h+28)!=total||
    !wf_cropped_atlas_word(h+32)||wf_cropped_atlas_word(h+32)>495u||
    !wf_ta_zero(h+36,2008)||wf_cropped_atlas_hash(h,2044)!=wf_cropped_atlas_word(h+2044))return -3;
 uint8_t entry[32];
 if(read(user,2048u+key*32u,entry,sizeof entry))return -2;
 if(wf_ta_zero(entry,sizeof entry))return 0;
 uint32_t at=wf_cropped_atlas_word(entry+4),n=wf_cropped_atlas_word(entry+8);
 if(wf_cropped_atlas_word(entry)!=key||(at&2047u)||at<WF_TAG_ATLAS_ARCHIVE_DATA||
    at>total||n>total-at||n<2048u+WF_CROPPED_ATLAS_MAP_BYTES||wf_cropped_atlas_word(entry+24)||
    wf_cropped_atlas_hash(entry,28)!=wf_cropped_atlas_word(entry+28))return -4;
 if(read(user,at,h,2048))return -2;
 size_t memory;
 if(wf_cropped_atlas_hash(h,2048)!=wf_cropped_atlas_word(entry+12)||
    !wf_cropped_atlas_extent(h,2048,cart_capacity,&memory)||memory!=n-2048u||
    wf_cropped_atlas_word(entry+16)!=wf_cropped_atlas_word(h+16)||
    wf_cropped_atlas_word(entry+20)!=memory)return -5;
 a->read=read;a->user=user;a->offset=at;a->bytes=n;a->key=key;a->active=true;return 1;
}
static int wf_tag_atlas_archive_read(const wf_tag_atlas_archive_t*a,uint32_t at,void*dst,size_t n){
 if(!a||!a->active||!a->read||(!dst&&n)||at>a->bytes||n>a->bytes-at||
    wf_ta_overlap(a,sizeof *a,dst,n)||at>UINT32_MAX-a->offset)return -1;
 if(!n)return 0;
 return a->read(a->user,a->offset+at,dst,n)?-2:0;
}
#endif
