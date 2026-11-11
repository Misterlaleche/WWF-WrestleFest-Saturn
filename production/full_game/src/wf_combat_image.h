#ifndef WF_COMBAT_IMAGE_H
#define WF_COMBAT_IMAGE_H
#include <stdbool.h>
#include <stdint.h>
#ifndef WF_COMBAT_IMAGE_RAM_END
#define WF_COMBAT_IMAGE_RAM_END 0x060ff000u
#endif
static inline uint32_t wf_combat_be32(const uint8_t*p){return (uint32_t)p[0]<<24|(uint32_t)p[1]<<16|(uint32_t)p[2]<<8|p[3];}
static inline uint32_t wf_combat_image_hash(const uint8_t*p,uint32_t n){
 uint32_t hash=2166136261u;while(n--)hash=(hash^*p++)*16777619u;return hash;
}
static inline bool wf_combat_image_header(const uint8_t h[32],uint32_t bytes){
 uint32_t main=wf_combat_be32(h+16),tail=wf_combat_be32(h+20),end=wf_combat_be32(h+24);
 return bytes>=8192u&&bytes<=0xc0000u&&!(bytes&3u)&&
  wf_combat_be32(h)==0x57464d33u&&wf_combat_be32(h+4)==bytes&&wf_combat_be32(h+8)==0x06004000u&&
  main>=8192u&&main<=bytes&&end==bytes-main&&!((main|tail|end)&3u)&&
  tail>=0x06004000u+main&&tail<=WF_COMBAT_IMAGE_RAM_END&&end<=WF_COMBAT_IMAGE_RAM_END-tail&&
  wf_combat_be32(h+28)==wf_combat_image_hash(h,28);
}
#endif
