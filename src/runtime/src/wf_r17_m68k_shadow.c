#include "wf_r17_m68k_shadow.h"

static int reg_ok(const wf_r17_m68k_shadow_t *r,unsigned reg){return r!=0 && reg<8u;}
static uint32_t ror32_low3(uint32_t v,unsigned count){
    unsigned i;count&=7u;
    for(i=0u;i<count;i++)v=(v>>1)|(v<<31);
    return v;
}
void wf_r17_m68k_software_reset(wf_r17_m68k_shadow_t *r){
    unsigned i;if(!r)return;for(i=0u;i<8u;i++)r->d[i]=0u;r->valid=1u;
}
void wf_r17_m68k_move_b(wf_r17_m68k_shadow_t *r,unsigned reg,uint8_t value){if(reg_ok(r,reg))r->d[reg]=(r->d[reg]&0xFFFFFF00u)|(uint32_t)value;}
void wf_r17_m68k_move_w(wf_r17_m68k_shadow_t *r,unsigned reg,uint16_t value){if(reg_ok(r,reg))r->d[reg]=(r->d[reg]&0xFFFF0000u)|(uint32_t)value;}
void wf_r17_m68k_move_l(wf_r17_m68k_shadow_t *r,unsigned reg,uint32_t value){if(reg_ok(r,reg))r->d[reg]=value;}
void wf_r17_m68k_moveq(wf_r17_m68k_shadow_t *r,unsigned reg,int8_t value){if(reg_ok(r,reg))r->d[reg]=(uint32_t)(int32_t)value;}
void wf_r17_m68k_clr_l(wf_r17_m68k_shadow_t *r,unsigned reg){if(reg_ok(r,reg))r->d[reg]=0u;}
uint32_t wf_r17_rng21b4_step(uint32_t *state,uint32_t g80,uint32_t d[8]){
    uint32_t x;unsigned i,rot;if(!state||!d)return 0u;
    x=*state;for(i=1u;i<8u;i++)x+=d[i];x+=g80;
    rot=(unsigned)((uint16_t)x&7u);x=ror32_low3(x,rot);
    *state=x;d[0]=x;return x;
}
uint32_t wf_r17_rng21b4_live(uint32_t *state,uint32_t g80,wf_r17_m68k_shadow_t *r){
    if(!state||!r||!r->valid)return 0u;
    return wf_r17_rng21b4_step(state,g80,r->d);
}
