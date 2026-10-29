#include "wf_r18_match_support.h"
#include <string.h>
#define TRY(expression) do { wf_status_t status=(expression);if(status!=WF_OK)return status; } while(0)
static uint16_t rw(const uint8_t *p) { return (uint16_t)(((unsigned)p[0]<<8)|p[1]); }
static uint16_t d(wf_r18_match_support_t *c,unsigned n) { return (uint16_t)c->machine->regs->d[n]; }
static void dw(wf_r18_match_support_t *c,unsigned n,uint16_t v) { wf_r17_m68k_move_w(c->machine->regs,n,v); }
static void q(wf_r18_match_support_t *c,unsigned n,int8_t v) { wf_r17_m68k_moveq(c->machine->regs,n,v); }
static wf_status_t block(wf_r18_match_support_t *c,uint32_t at) { c->machine->dependency=at;return WF_BLOCKED; }
static wf_status_t read(wf_r18_match_support_t *c,uint32_t at,unsigned count,uint32_t *out) {
    uint8_t b[4];
    if(at>0x80000u||count>0x80000u-at||c->read_rom(c->user,at,b,count)!=WF_OK)return block(c,at);
    *out=0;for(unsigned i=0;i<count;i++)*out=(*out<<8)|b[i];return WF_OK;
}
static wf_status_t fg(wf_r18_match_support_t *c,uint32_t at,uint8_t value) {
    if(at<0xc0000u||at-0xc0000u>=c->fg_bytes)return block(c,at);
    c->fg_0c0000[at-0xc0000u]=value;return WF_OK;
}
static wf_status_t pal(wf_r18_match_support_t *c,uint32_t at,uint16_t value) {
    if(at<0x180000u||at-0x180000u>0x9000u-2u)return block(c,at);
    if(c->write_palette) {
        if(c->write_palette(c->user,at,value)!=WF_OK)return block(c,at);
    } else { uint8_t *p=c->palette_180000+at-0x180000u;p[0]=(uint8_t)(value>>8);p[1]=(uint8_t)value; }
    return WF_OK;
}
static wf_status_t effect(wf_r18_match_support_t *c,uint32_t owner) {
    c->machine->dependency=owner;
    if(!c->machine->effect)return WF_BLOCKED;
    wf_status_t status=c->machine->effect(c->machine->user,owner,c->machine);
    if(status==WF_OK)c->machine->dependency=0;
    return status;
}
static wf_status_t layout_993a(wf_r18_match_support_t *c,const uint8_t *p) {
    uint32_t *a=c->machine->a,value;a[1]=0x9980u;a[2]=0xc0000u;q(c,1,0);
    if(d(c,0)&1u)q(c,1,1);
    if(p[0x33]&0x80u)dw(c,1,(uint16_t)(d(c,1)+2u));
    dw(c,7,d(c,1));dw(c,7,(uint16_t)(d(c,7)+((d(c,1)&2u)?12u:6u)));dw(c,7,(uint16_t)(d(c,7)<<4));
    dw(c,4,d(c,1));dw(c,4,(uint16_t)(d(c,4)<<1));q(c,3,0);dw(c,3,d(c,2));dw(c,3,(uint16_t)(d(c,3)<<3));
    dw(c,3,(uint16_t)(d(c,3)+d(c,4)));TRY(read(c,a[1]+(int16_t)d(c,3),2,&value));dw(c,3,(uint16_t)value);
    a[2]+=c->machine->regs->d[3];return WF_OK;
}
static wf_status_t special_99d0(wf_r18_match_support_t *c,const uint8_t *p) {
    uint32_t saved_d[4],saved_a[3],value;memcpy(saved_d,c->machine->regs->d,sizeof saved_d);memcpy(saved_a,c->machine->a+1,sizeof saved_a);
    uint32_t *a=c->machine->a;a[2]=0xc0000u;dw(c,0,p[0x33]&0x80u?0x1504u:0x504u);a[2]+=c->machine->regs->d[0];
    dw(c,7,(uint16_t)(d(c,7)+9u));a[1]=0x9d36u;q(c,0,7);
    do {
        a[3]=a[2];q(c,1,0);
        do {
            dw(c,2,d(c,1));dw(c,2,(uint16_t)(d(c,2)<<1));
            TRY(read(c,a[1]+(int16_t)d(c,2),1,&value));wf_r17_m68k_move_b(c->machine->regs,3,(uint8_t)value);
            TRY(fg(c,a[3],(uint8_t)d(c,3)));TRY(fg(c,a[3]+3u,(uint8_t)d(c,7)));a[3]+=4u;
            TRY(read(c,a[1]+(int16_t)d(c,2)+1u,1,&value));wf_r17_m68k_move_b(c->machine->regs,3,(uint8_t)value);
            TRY(fg(c,a[3],(uint8_t)d(c,3)));TRY(fg(c,a[3]+3u,(uint8_t)d(c,7)));a[3]+=4u;dw(c,1,(uint16_t)(d(c,1)+1u));
        } while(d(c,1)!=19u);
        a[1]+=0x26u;a[2]+=0x100u;dw(c,0,(uint16_t)(d(c,0)-1u));
    } while(d(c,0)!=0xffffu);
    memcpy(c->machine->regs->d,saved_d,sizeof saved_d);memcpy(c->machine->a+1,saved_a,sizeof saved_a);return WF_OK;
}
static wf_status_t palette_9a78(wf_r18_match_support_t *c) {
    uint32_t saved_d[4],saved_a[2],value;memcpy(saved_d,c->machine->regs->d,sizeof saved_d);memcpy(saved_a,c->machine->a+1,sizeof saved_a);
    uint32_t *a=c->machine->a;a[1]=0x9e66u;dw(c,2,(uint16_t)(d(c,2)<<2));TRY(read(c,a[1]+(int16_t)d(c,2),4,&value));a[1]=value;
    a[2]=d(c,1)&2u?0x180700u:0x180300u;dw(c,0,0x80u);if(!(d(c,1)&1u))q(c,0,0);a[2]+=c->machine->regs->d[0];q(c,0,0);
    do {
        dw(c,3,d(c,0));dw(c,3,(uint16_t)(d(c,3)<<1));TRY(read(c,a[1]+(int16_t)d(c,3),2,&value));dw(c,3,(uint16_t)value);
        TRY(pal(c,a[2],d(c,3)));a[2]+=2u;dw(c,0,(uint16_t)(d(c,0)+1u));
    } while(d(c,0)!=16u);
    memcpy(c->machine->regs->d,saved_d,sizeof saved_d);memcpy(c->machine->a+1,saved_a,sizeof saved_a);return WF_OK;
}
static wf_status_t join_98ba(wf_r18_match_support_t *c) {
    uint32_t *a=c->machine->a;q(c,0,0);a[0]=0x1c05b0u;
    do {
        uint8_t *p=c->machine->ram+a[0]-0x1c0000u;
        if(p[0]&0x80u) {
            dw(c,2,(uint16_t)(rw(p+0x56)&15u));TRY(layout_993a(c,p));a[3]=0xc0e40u;
            if(d(c,2)==4u||d(c,2)==5u) { if(!(p[0x33]&0x80u))a[3]=0xc1040u;a[3]-=0x100u; }
            if(d(c,2)>=10u)TRY(special_99d0(c,p));else TRY(effect(c,0x9acau));
            TRY(palette_9a78(c));
        }
        a[0]+=0x10cu;dw(c,0,(uint16_t)(d(c,0)+1u));
    } while(d(c,0)!=9u);
    q(c,0,0);a[2]=a[3];dw(c,2,12u);q(c,7,0);return effect(c,0x9acau);
}
wf_status_t wf_r18_match_support_call(wf_r18_match_support_t *c,uint32_t owner) {
    if(!c||!c->machine||!c->machine->ram||!c->machine->regs||!c->machine->regs->valid)return WF_BAD_STATE;
    c->machine->dependency=0;
    if(owner==0x8eb6u) { dw(c,0,0x3100u);TRY(effect(c,0x2052u));dw(c,0,0x3120u);return effect(c,0x2052u); }
    if(owner!=0x98bau)return block(c,owner);
    if(c->machine->ram[0x161]&1u)return WF_OK;
    if(!c->fg_0c0000||c->fg_bytes<0x4000u||(!c->write_palette&&(!c->palette_180000||c->palette_bytes<0x9000u))||!c->read_rom)return WF_BAD_STATE;
    return join_98ba(c);
}
