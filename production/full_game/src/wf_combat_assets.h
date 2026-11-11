#ifndef WF_COMBAT_ASSETS_H
#define WF_COMBAT_ASSETS_H
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
/* Immutable resources follow the active original participants. Reading this
 * key never changes character selection, actor state, or RNG. */
static inline unsigned wf_combat_asset_mask(const void *data,unsigned count,size_t stride){
 const uint8_t *p=data;unsigned mask=0;
 if(!p||stride<4)return 0;
 for(unsigned i=0;i<count;i++,p+=stride){
  unsigned id=(unsigned)p[2]<<8|p[3];
  if((p[0]&0x80u)&&id<12u)mask|=1u<<id;
 }
 return mask;
}
static inline bool wf_combat_asset_name(char out[13],unsigned royal,unsigned mask,char kind){
 static const char hex[]="0123456789ABCDEF";
 if(!out||!mask||mask>0xfffu||kind<'A'||kind>'Z')return false;
 out[0]=royal?'R':'T';out[1]=hex[mask>>8];out[2]=hex[(mask>>4)&15];out[3]=hex[mask&15];
 out[4]=kind;out[5]='.';out[6]='B';out[7]='I';out[8]='N';out[9]=0;return true;
}
#endif
