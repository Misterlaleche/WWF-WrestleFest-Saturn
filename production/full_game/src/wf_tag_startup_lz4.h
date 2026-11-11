#ifndef WF_TAG_STARTUP_LZ4_H
#define WF_TAG_STARTUP_LZ4_H
#include "wf_tag_atlas_archive.h"
/* Startup transport only. Decode into the final immutable cart atlas, using
 * the checked unread packed tail temporarily. No decode or CD reader survives into
 * combat; original R19C bind still checks the complete map/pixel checksum. */
enum {WF_TAG_LZ4_BLOCK=65536u};
typedef int (*wf_tag_lz4_decode_fn)(const uint8_t*,size_t,uint8_t*,size_t);
typedef struct {uint8_t header[2048];uint8_t*data;uint32_t memory,key;bool active;}wf_tag_lz4_t;
static inline int wf_tag_lz4_open(wf_tag_lz4_t*c,wf_tag_atlas_archive_read_fn read,void*user,
 uint32_t total,uint32_t key,void*cart,size_t capacity,void*scratch,size_t scratch_bytes,wf_tag_lz4_decode_fn decode){
 if(!c)return -1;memset(c,0,sizeof *c);
 if(!read||!decode||!cart||!scratch||scratch_bytes<2048||capacity!=0x400000u||
    (uintptr_t)cart>UINTPTR_MAX-capacity||!wf_ta_key(key)||total<WF_TAG_ATLAS_ARCHIVE_DATA||(total&2047u)||
    wf_ta_overlap(c,sizeof *c,cart,capacity)||wf_ta_overlap(c,sizeof *c,scratch,2048)||
    wf_ta_overlap(cart,capacity,scratch,2048))return -1;
 uint8_t*d=scratch;uint8_t e[32];
 if(read(user,0,d,2048))return -2;
 if(memcmp(d,"WTZ1",4)||wf_cropped_atlas_word(d+4)!=1||wf_cropped_atlas_word(d+8)!=2048||
    wf_cropped_atlas_word(d+12)!=4096||wf_cropped_atlas_word(d+16)!=32||wf_cropped_atlas_word(d+20)!=2048||
    wf_cropped_atlas_word(d+24)!=WF_TAG_ATLAS_ARCHIVE_DATA||wf_cropped_atlas_word(d+28)!=total||
    !wf_cropped_atlas_word(d+32)||wf_cropped_atlas_word(d+32)>495||wf_cropped_atlas_word(d+36)!=WF_TAG_LZ4_BLOCK||
    !wf_ta_zero(d+40,2004)||wf_cropped_atlas_hash(d,2044)!=wf_cropped_atlas_word(d+2044))return -3;
 if(read(user,2048+key*32,e,32))return -2;
 if(wf_ta_zero(e,32))return 0;
 uint32_t at=wf_cropped_atlas_word(e+4),bytes=wf_cropped_atlas_word(e+8),raw=wf_cropped_atlas_word(e+12),memory=wf_cropped_atlas_word(e+20);
 if(wf_cropped_atlas_word(e)!=key||wf_cropped_atlas_word(e+24)||wf_cropped_atlas_hash(e,28)!=wf_cropped_atlas_word(e+28)||
    at<WF_TAG_ATLAS_ARCHIVE_DATA||(at&2047u)||at>total||bytes>total-at||bytes<6144||(bytes&2047u)||
    memory<WF_CROPPED_ATLAS_MAP_BYTES||memory>capacity-WF_TAG_LZ4_BLOCK||raw!=memory+2048u)return -4;
 if(read(user,at,d,2048)||read(user,at+2048,c->header,2048))return -2;
 uint32_t count=(memory+WF_TAG_LZ4_BLOCK-1)/WF_TAG_LZ4_BLOCK;size_t extent;
 if(memcmp(d,"TZP1",4)||wf_cropped_atlas_word(d+4)!=1||wf_cropped_atlas_word(d+8)!=bytes||
    wf_cropped_atlas_word(d+12)!=memory||wf_cropped_atlas_word(d+16)!=WF_TAG_LZ4_BLOCK||wf_cropped_atlas_word(d+20)!=count||
    wf_cropped_atlas_word(d+24)!=wf_cropped_atlas_word(e+16)||wf_cropped_atlas_word(d+28)!=key||
    !wf_ta_zero(d+32,32)||!wf_ta_zero(d+64+count*16,2044-64-count*16)||
    wf_cropped_atlas_hash(d,2044)!=wf_cropped_atlas_word(d+2044)||
    wf_cropped_atlas_hash(c->header,2048)!=wf_cropped_atlas_word(e+16)||
    !wf_cropped_atlas_extent(c->header,2048,capacity,&extent)||extent!=memory)return -5;
 /* Load the whole compressed body at the cart's upper end. Each output
  * block must finish before its current packed input starts. This preflight
  * proves forward expansion cannot destroy current or future source bytes. */
 uint32_t payload=bytes-4096u;
 if(payload>capacity)return -8;
 size_t packed_base=capacity-payload;
 uint32_t end=4096,done=0;
 for(uint32_t i=0;i<count;i++){
  const uint8_t*b=d+64+i*16;uint32_t off=wf_cropped_atlas_word(b),in=wf_cropped_atlas_word(b+4),out=wf_cropped_atlas_word(b+8),codec=wf_cropped_atlas_word(b+12);
  uint32_t expect=memory-done;if(expect>WF_TAG_LZ4_BLOCK)expect=WF_TAG_LZ4_BLOCK;
  if(off!=end||!in||in>out||out!=expect||codec>1||(!codec&&in!=out)||off>bytes||((in+2047u)&~2047u)>bytes-off)return -6;
  if((size_t)done+out>packed_base+off-4096u)return -8;
  end=off+((in+2047u)&~2047u);done+=out;
 }
 if(end!=bytes||done!=memory)return -6;
 uint8_t*packed=(uint8_t*)cart+packed_base;
 if(read(user,at+4096u,packed,payload))return -2;
 done=0;
 for(uint32_t i=0;i<count;i++){
  const uint8_t*b=d+64+i*16;uint32_t off=wf_cropped_atlas_word(b),in=wf_cropped_atlas_word(b+4),out=wf_cropped_atlas_word(b+8);
  const uint8_t*input=packed+(off-4096u);
  if(wf_cropped_atlas_word(b+12)){if(decode(input,in,(uint8_t*)cart+done,out))return -7;}
  else memcpy((uint8_t*)cart+done,input,out);
  done+=out;
 }
 c->data=cart;c->memory=memory;c->key=key;c->active=true;return 1;
}
static inline int wf_tag_lz4_read(void*user,uint32_t at,void*dst,size_t n){
 const wf_tag_lz4_t*c=user;
 if(!c||!c->active||!c->data||(!dst&&n)||at>c->memory+2048u||n>c->memory+2048u-at||
    wf_ta_overlap(c,sizeof *c,dst,n))return -1;
 if(!n)return 0;
 /* The original provider loads the body into this exact final destination.
  * Any other write alias would corrupt the immutable source for later reads. */
 if(wf_ta_overlap(c->data,c->memory,dst,n))return at>=2048u&&dst==c->data+at-2048u?0:-1;
 uint8_t*out=dst;
 if(at<2048u){size_t take=2048u-at;if(take>n)take=n;memcpy(out,c->header+at,take);out+=take;at+=(uint32_t)take;n-=take;}
 if(n)memcpy(out,c->data+at-2048u,n);
 return 0;
}
#endif
