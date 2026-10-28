#include "wf_target_relation.h"
#include <string.h>
static uint16_t rw(const uint8_t*p){return (uint16_t)((p[0]<<8)|p[1]);}
static uint32_t rl(const uint8_t*p){return ((uint32_t)rw(p)<<16)|rw(p+2);}
static void ww(uint8_t*p,uint16_t v){p[0]=(uint8_t)(v>>8);p[1]=(uint8_t)v;}
static void wl(uint8_t*p,uint32_t v){ww(p,(uint16_t)(v>>16));ww(p+2,(uint16_t)v);}
int royal_target_oracle(uint8_t*mem,uint32_t d[8]){
 static wf_runtime_t r;memset(&r,0,sizeof r);r.m68k_regs.valid=1;memcpy(r.m68k_regs.d,d,32);
 uint8_t*p=mem+0x1c0000;r.g.mode_0161=p[0x161];r.g.global_168c=rl(p+0x168c);r.g.global_1692=rw(p+0x1692);
 memcpy(r.actors,p+0x5b0,sizeof r.actors);int st=wf_phase_target_relation_20958(&r,0x20958,NULL);
 memcpy(p+0x5b0,r.actors,sizeof r.actors);wl(p+0x168c,r.g.global_168c);ww(p+0x1692,r.g.global_1692);memcpy(d,r.m68k_regs.d,32);return st;
}
