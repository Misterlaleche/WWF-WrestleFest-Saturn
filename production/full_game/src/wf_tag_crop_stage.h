#ifndef WF_TAG_CROP_STAGE_H
#define WF_TAG_CROP_STAGE_H
#include <stddef.h>
#include <stdint.h>
#include <string.h>

/* Synchronous startup borrower only: no atlas, scene or cart ROM is live.
 * The canonical uncached 4MB cart is already physically validated by platform.
 * Nothing in the lease or staged file survives this call. */
enum { WF_TAG_CROP_HINT_BYTES=131072u,WF_TAG_CROP_FILE_BYTES=131080u,
       WF_TAG_CROP_STAGE_BYTES=131104u };
static inline int wf_tag_crop_stage_lease(uintptr_t cart,size_t capacity,
 uintptr_t hints,size_t hint_bytes,uintptr_t*out){
 if(!out||cart!=0x22400000u||capacity!=0x400000u||
    hints!=0x00221000u||hint_bytes!=WF_TAG_CROP_HINT_BYTES||
    capacity>UINTPTR_MAX-cart)return 0;
 uintptr_t end=cart+capacity,stage=end-WF_TAG_CROP_STAGE_BYTES;
 if((stage&31u)||stage<cart||WF_TAG_CROP_STAGE_BYTES>end-stage)return 0;
 *out=stage;return 1;
}
/* SH-2 P0/P1 addresses alias the same physical bus. Check those intervals
 * before dereferencing; wider host pointers retain their native identity. */
static inline int wf_tag_crop_stage_span(uintptr_t p,size_t n,uintptr_t*physical){
 if(!p||n>UINTPTR_MAX-p)return 0;
 if(p<=UINT32_MAX){
  p&=0x1fffffffu;
  if(n>0x20000000u-p)return 0;
 }
 *physical=p;return 1;
}
static inline int wf_tag_crop_stage_load(void*staging,size_t stage_bytes,
 void*destination,size_t hint_bytes,uint32_t file_bytes,
 int(*read_file)(const char*,uint32_t,void*,size_t)){
 uintptr_t src=(uintptr_t)staging,dst=(uintptr_t)destination,a,b;
 if(!read_file||file_bytes!=WF_TAG_CROP_FILE_BYTES||stage_bytes<WF_TAG_CROP_STAGE_BYTES||
    hint_bytes!=WF_TAG_CROP_HINT_BYTES||(src&31u)||(dst&31u)||
    !wf_tag_crop_stage_span(src,stage_bytes,&a)||
    !wf_tag_crop_stage_span(dst,hint_bytes,&b)||
    (a<b+hint_bytes&&b<a+stage_bytes))return 0;
 if(read_file("CROPALL.BIN",0,staging,WF_TAG_CROP_FILE_BYTES))return 0;
 const uint8_t*header=(const uint8_t*)staging,*hints=header+8;
 if(memcmp(header,"R18A",4))return 0;
 uint32_t hash=2166136261u;
 for(unsigned i=0;i<WF_TAG_CROP_HINT_BYTES;i++)hash=(hash^hints[i])*16777619u;
 if(hash!=((uint32_t)header[4]<<24|(uint32_t)header[5]<<16|
           (uint32_t)header[6]<<8|header[7]))return 0;
 for(unsigned i=0;i<65536u;i++){
  unsigned meta=hints[i*2],height=hints[i*2+1],top=meta&31u;
  if((meta&128u)||top>16u||height>16u||top+height>16u||
     ((meta&64u)&&!(meta&32u)))return 0;
 }
 /* No partial final hints on I/O, integrity or geometry failure. */
 memcpy(destination,hints,WF_TAG_CROP_HINT_BYTES);return 1;
}
#endif
