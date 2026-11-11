#ifndef WF_CROP_COPY_H
#define WF_CROP_COPY_H
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
static inline void wf_crop_copy_words(uint8_t *dst,const uint8_t *src,unsigned bytes){
 if(((uintptr_t)dst|(uintptr_t)src)&3u){memcpy(dst,src,bytes);return;}
 for(unsigned at=0;at<bytes;at+=4u){
  uint32_t word;
  __builtin_memcpy(&word,__builtin_assume_aligned(src+at,4u),4u);
  __builtin_memcpy(__builtin_assume_aligned(dst+at,4u),&word,4u);
 }
}
/* Copy a verified original crop into the backend's existing 128-byte slot.
 * Wide tiles retain their row offset; narrow tiles start at slot byte zero.
 * Source and destination are separate 128-byte regions. */
static inline bool wf_crop_copy(uint8_t *dst,const uint8_t *src,uint16_t crop){
 unsigned meta=crop>>8,top=meta&31u,height=crop&255u;
 if(!dst||!src||(meta&128u)||top>16u||height>16u||top+height>16u||
    ((meta&64u)&&!(meta&32u)))return false;
 if(meta&32u){
  unsigned column=(meta&64u)?4u:0u;
  for(unsigned y=0;y<height;y++)
   wf_crop_copy_words(dst+y*4u,src+(top+y)*8u+column,4u);
  memset(dst+height*4u,0,128u-height*4u);
 }else{
  unsigned first=top*8u,last=(top+height)*8u;
  memset(dst,0,first);
  wf_crop_copy_words(dst+first,src+first,height*8u);
  memset(dst+last,0,128u-last);
 }
 return true;
}
#endif
