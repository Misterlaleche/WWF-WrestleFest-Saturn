#ifndef WF_TILE_INDEX_H
#define WF_TILE_INDEX_H
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
enum {WF_TILE_INDEX_BYTES=12288,WF_TILE_INDEX_SCRATCH=8192};
static inline unsigned wf_tile_index_popcount(uint32_t v){
 v-=((v>>1)&0x55555555u);
 v=(v&0x33333333u)+((v>>2)&0x33333333u);
 v=(v+(v>>4))&0x0f0f0f0fu;v+=v>>8;v+=v>>16;return v&63u;
}
/* Replace a validated sorted BE16 ID list in place: 2048 rank prefixes
 * followed by a native-endian 65536-bit membership map. The compressed
 * block-offset directory following the original IDs is never touched.
 * Build is confined to hidden loading, using the idle DMA staging area. */
static inline bool wf_tile_index_build(uint8_t *ids,size_t count,void *scratch,size_t scratch_bytes){
 if(!ids||!scratch||count<WF_TILE_INDEX_BYTES/2||count>32768u||
    scratch_bytes<WF_TILE_INDEX_SCRATCH||((uintptr_t)ids&3u))return false;
 uintptr_t a=(uintptr_t)ids,b=(uintptr_t)scratch;
 if(a>UINTPTR_MAX-count*2u||b>UINTPTR_MAX-WF_TILE_INDEX_SCRATCH||
    (a<b+WF_TILE_INDEX_SCRATCH&&b<a+count*2u))return false;
 uint8_t *bits=scratch;memset(bits,0,WF_TILE_INDEX_SCRATCH);
 unsigned previous=0;
 for(size_t i=0;i<count;i++){
  unsigned tile=(unsigned)ids[i*2]<<8|ids[i*2+1];
  if(i&&tile<=previous)return false;
  previous=tile;
  uint32_t word;__builtin_memcpy(&word,bits+(tile>>5)*4u,4);
  word|=1u<<(tile&31u);__builtin_memcpy(bits+(tile>>5)*4u,&word,4);
 }
 unsigned rank=0;
 for(unsigned group=0;group<2048;group++){
  ids[group*2]=(uint8_t)(rank>>8);ids[group*2+1]=(uint8_t)rank;
  uint32_t word;__builtin_memcpy(&word,bits+group*4u,4);rank+=wf_tile_index_popcount(word);
 }
 memcpy(ids+4096,bits,WF_TILE_INDEX_SCRATCH);return true;
}
/* Only a successfully built, four-byte-aligned index may be queried. */
static inline bool wf_tile_index_find(const uint8_t *index,uint16_t tile,unsigned *rank){
 const uint8_t *data=__builtin_assume_aligned(index,4);
 unsigned group=(unsigned)tile>>5;uint32_t word,bit=1u<<(tile&31u);
 __builtin_memcpy(&word,data+4096+group*4u,4);
 if(!(word&bit))return false;
 *rank=((unsigned)data[group*2]<<8|data[group*2+1])+wf_tile_index_popcount(word&(bit-1u));
 return true;
}
#endif
