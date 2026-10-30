#include "wf_r18_match_hud.h"
#include <string.h>
#define TRY(expression) do { wf_status_t status=(expression);if(status!=WF_OK)return status; } while(0)
static uint16_t rw(const uint8_t *p) { return (uint16_t)(((unsigned)p[0]<<8)|p[1]); }
static void ww(uint8_t *p,uint16_t v) { p[0]=(uint8_t)(v>>8);p[1]=(uint8_t)v; }
static uint16_t d(wf_r18_match_hud_t *c,unsigned n) { return (uint16_t)c->machine->regs->d[n]; }
static void dw(wf_r18_match_hud_t *c,unsigned n,uint16_t v) { wf_r17_m68k_move_w(c->machine->regs,n,v); }
static void q(wf_r18_match_hud_t *c,unsigned n,int8_t v) { wf_r17_m68k_moveq(c->machine->regs,n,v); }
static wf_status_t block(wf_r18_match_hud_t *c,uint32_t at) { c->machine->dependency=at;return WF_BLOCKED; }
static wf_status_t read(wf_r18_match_hud_t *c,uint32_t at,unsigned bytes,uint32_t *out) {
    uint8_t b[4];
    if(at>0x80000u||bytes>0x80000u-at||c->read_rom(c->user,at,b,bytes)!=WF_OK)return block(c,at);
    *out=0;for(unsigned i=0;i<bytes;i++)*out=(*out<<8)|b[i];return WF_OK;
}
static wf_status_t fg(wf_r18_match_hud_t *c,uint32_t at,uint8_t v) {
    if(at<0xc0000u||at-0xc0000u>=c->fg_bytes)return block(c,at);
    c->fg_0c0000[at-0xc0000u]=v;return WF_OK;
}
static wf_status_t pal(wf_r18_match_hud_t *c,uint32_t at,uint16_t v) {
    if(at<0x180000u||at-0x180000u>0x9000u-2u)return block(c,at);
    if(c->write_palette) {
        if(c->write_palette(c->user,at,v)!=WF_OK)return block(c,at);
        return WF_OK;
    }
    ww(c->palette_180000+at-0x180000u,v);return WF_OK;
}
static uint8_t *actor(wf_r18_match_hud_t *c,uint32_t address) {
    if(address<0x1c0000u||address-0x1c0000u>0x4000u-0x90u) { block(c,address);return NULL; }
    return c->machine->ram+address-0x1c0000u;
}
/* $7720 returns carry for one of the first four exact actor addresses. */
static int slot(wf_r18_match_hud_t *c) {
    uint32_t *a=c->machine->a;q(c,0,0);a[1]=0x1c05b0u;
    do { if(a[0]==a[1])return 1;a[1]+=0x10cu;dw(c,0,(uint16_t)(d(c,0)+1u)); } while(d(c,0)<4u);
    return 0;
}
static wf_status_t base_7680(wf_r18_match_hud_t *c) {
    uint32_t saved=c->machine->regs->d[0],value;slot(c);c->machine->a[1]=0x76a2u;
    dw(c,0,(uint16_t)(d(c,0)<<1));TRY(read(c,c->machine->a[1]+(int16_t)d(c,0),2,&value));dw(c,0,(uint16_t)value);
    c->machine->a[1]=0xc0000u+c->machine->regs->d[0];c->machine->regs->d[0]=saved;return WF_OK;
}
static wf_status_t tile_76e8(wf_r18_match_hud_t *c) {
    TRY(fg(c,c->machine->a[1]+1u,(uint8_t)d(c,4)));dw(c,4,(uint16_t)(d(c,4)>>8));
    dw(c,4,(uint16_t)(d(c,4)|0x10u));return fg(c,c->machine->a[1]+3u,(uint8_t)d(c,4));
}
static wf_status_t bar_76aa(wf_r18_match_hud_t *c) {
    uint32_t saved[4];memcpy(saved,c->machine->regs->d+1,sizeof saved);
    q(c,4,0);q(c,2,7);dw(c,3,0xb33u);
    if(d(c,1)) { q(c,2,11);dw(c,3,0xad9u);if(d(c,0)&2u) { q(c,2,14);dw(c,3,0xaf2u); } }
    dw(c,3,(uint16_t)(d(c,3)+d(c,1)));dw(c,4,d(c,3));TRY(tile_76e8(c));
    c->machine->a[1]+=0x100u;dw(c,3,(uint16_t)(d(c,3)+d(c,2)));dw(c,4,d(c,3));TRY(tile_76e8(c));
    c->machine->a[1]-=0x100u;memcpy(c->machine->regs->d+1,saved,sizeof saved);return WF_OK;
}
static wf_status_t health_75ee(wf_r18_match_hud_t *c) {
    uint32_t saved[6];memcpy(saved,c->machine->a,sizeof saved);TRY(base_7680(c));
    c->machine->a[1]+=0x20cu;dw(c,2,5u);
    do { q(c,1,0);TRY(bar_76aa(c));c->machine->a[1]+=4u;dw(c,2,(uint16_t)(d(c,2)-1u)); } while(d(c,2)!=0xffffu);
    uint8_t *p=actor(c,c->machine->a[0]);if(!p)return WF_BLOCKED;
    wf_r17_m68k_move_b(c->machine->regs,1u,p[0x6f]);c->machine->regs->d[1]&=255u;
    if(c->machine->regs->d[1]) {
        unsigned value=c->machine->regs->d[1];c->machine->regs->d[1]=((value%16u)<<16)|(value/16u);
        c->machine->regs->d[2]=c->machine->regs->d[1];TRY(base_7680(c));c->machine->a[1]+=0x20cu;q(c,3,1);
        c->machine->regs->d[1]&=0xffffu;
        if(d(c,1)) {
            if(d(c,1)>=6u) { dw(c,1,6u);q(c,3,0); }
            dw(c,1,(uint16_t)(d(c,1)-1u));
            do {
                uint32_t saved_d1=c->machine->regs->d[1];q(c,1,8);TRY(bar_76aa(c));c->machine->regs->d[1]=saved_d1;
                c->machine->a[1]+=4u;dw(c,1,(uint16_t)(d(c,1)-1u));
            } while(d(c,1)!=0xffffu);
        }
        if(d(c,3)) {
            value=c->machine->regs->d[2];c->machine->regs->d[2]=(value<<16)|(value>>16);dw(c,2,(uint16_t)(d(c,2)&255u));
            if(d(c,2)) {
                if(d(c,2)&1u)dw(c,2,(uint16_t)(d(c,2)-1u));
                dw(c,2,(uint16_t)(d(c,2)>>1));dw(c,1,d(c,2));TRY(bar_76aa(c));
            }
        }
    }
    memcpy(c->machine->a,saved,sizeof saved);return WF_OK;
}
static wf_status_t portrait_77d8(wf_r18_match_hud_t *c) {
    uint32_t saved_d[4],saved_a[2],value;memcpy(saved_d,c->machine->regs->d,sizeof saved_d);memcpy(saved_a,c->machine->a,sizeof saved_a);
    c->machine->a[1]=0x7814u;dw(c,0,(uint16_t)(d(c,0)<<1));TRY(read(c,c->machine->a[1]+(int16_t)d(c,0),2,&value));dw(c,0,(uint16_t)value);
    q(c,2,2);
    do {
        q(c,3,2);
        do {
            TRY(fg(c,c->machine->a[0]+1u,(uint8_t)d(c,0)));dw(c,1,d(c,0));dw(c,1,(uint16_t)(d(c,1)>>8));
            TRY(fg(c,c->machine->a[0]+3u,(uint8_t)d(c,1)));dw(c,0,(uint16_t)(d(c,0)+1u));c->machine->a[0]+=4u;
            dw(c,3,(uint16_t)(d(c,3)-1u));
        } while(d(c,3)!=0xffffu);
        c->machine->a[0]+=0xf4u;dw(c,2,(uint16_t)(d(c,2)-1u));
    } while(d(c,2)!=0xffffu);
    memcpy(c->machine->regs->d,saved_d,sizeof saved_d);memcpy(c->machine->a,saved_a,sizeof saved_a);return WF_OK;
}
static wf_status_t portrait_782c(wf_r18_match_hud_t *c) {
    uint32_t saved_d0=c->machine->regs->d[0],saved_a[2];memcpy(saved_a,c->machine->a,sizeof saved_a);
    if(rw(c->machine->ram+0x7c)) {
        uint8_t *p=actor(c,c->machine->a[0]);if(!p)return WF_BLOCKED;
        if((p[0]&0x80u)&&!(p[0x56]&0x80u)&&slot(c)) {
            TRY(base_7680(c));c->machine->a[1]+=0x100u;dw(c,0,(uint16_t)(rw(p+0x56)&15u));
            if(d(c,0)!=5u) { c->machine->a[0]=c->machine->a[1];TRY(portrait_77d8(c)); }
        }
    }
    c->machine->regs->d[0]=saved_d0;memcpy(c->machine->a,saved_a,sizeof saved_a);return WF_OK;
}
static wf_status_t player_776c(wf_r18_match_hud_t *c) {
    uint32_t saved_d[5],saved_a[2],value;memcpy(saved_d,c->machine->regs->d,sizeof saved_d);memcpy(saved_a,c->machine->a,sizeof saved_a);
    c->machine->a[0]=0x1c05b0u;q(c,0,0);
    do {
        uint8_t *p=actor(c,c->machine->a[0]);if(!p)return WF_BLOCKED;
        if((p[0]&0x80u)&&!(p[0x56]&0x80u)) {
            c->machine->regs->d[1]=((uint32_t)rw(p+0x8a)<<16)|rw(p+0x8c);dw(c,1,(uint16_t)(d(c,1)&15u));
            c->machine->a[1]=0x77d0u;TRY(read(c,c->machine->a[1]+(int16_t)d(c,1),2,&value));dw(c,2,(uint16_t)value);
            TRY(base_7680(c));q(c,1,2);
            do { dw(c,4,d(c,2));TRY(tile_76e8(c));c->machine->a[1]+=4u;dw(c,2,(uint16_t)(d(c,2)+1u));dw(c,1,(uint16_t)(d(c,1)-1u)); } while(d(c,1)!=0xffffu);
        }
        c->machine->a[0]+=0x10cu;dw(c,0,(uint16_t)(d(c,0)+1u));
    } while(d(c,0)!=4u);
    memcpy(c->machine->regs->d,saved_d,sizeof saved_d);memcpy(c->machine->a,saved_a,sizeof saved_a);return WF_OK;
}
static wf_status_t hud_7506(wf_r18_match_hud_t *c) {
    uint32_t saved_d0=c->machine->regs->d[0],saved_a[2];memcpy(saved_a,c->machine->a,sizeof saved_a);
    c->machine->a[0]=0x1c05b0u;q(c,0,0);
    do {
        uint8_t *p=actor(c,c->machine->a[0]);if(!p)return WF_BLOCKED;
        if((p[0]&0x80u)&&!(p[0x56]&0x80u)) { ww(p+0x6e,rw(p+0x66));TRY(health_75ee(c));TRY(portrait_782c(c));TRY(player_776c(c)); }
        c->machine->a[0]+=0x10cu;dw(c,0,(uint16_t)(d(c,0)+1u));
    } while(d(c,0)!=4u);
    c->machine->regs->d[0]=saved_d0;memcpy(c->machine->a,saved_a,sizeof saved_a);return WF_OK;
}
static wf_status_t palette_c1ea(wf_r18_match_hud_t *c) {
    if(!rw(c->machine->ram+0x7c)||(c->machine->ram[0x161]&1u))return WF_OK;
    uint32_t saved_d[3],saved_a[3],value;memcpy(saved_d,c->machine->regs->d+1,sizeof saved_d);memcpy(saved_a,c->machine->a+1,sizeof saved_a);
    c->machine->a[1]=0x1c05b0u;q(c,1,3);
    do {
        uint8_t *p=actor(c,c->machine->a[1]);if(!p)return WF_BLOCKED;
        if(p[0]&0x80u) {
            dw(c,2,rw(p+0x56));c->machine->a[2]=p[0x33]&1u?0xc264u:0xc294u;
            c->machine->regs->d[2]&=15u;c->machine->a[3]=0x180100u;c->machine->regs->d[3]=c->machine->regs->d[2];
            dw(c,3,(uint16_t)(d(c,3)<<7));c->machine->a[3]+=c->machine->regs->d[3];dw(c,2,(uint16_t)(d(c,2)<<2));
            TRY(read(c,c->machine->a[2]+(int16_t)d(c,2),4,&value));c->machine->a[2]=value;q(c,2,15);
            do {
                dw(c,3,d(c,2));dw(c,3,(uint16_t)(d(c,3)<<1));TRY(read(c,c->machine->a[2]+(int16_t)d(c,3),2,&value));
                TRY(pal(c,c->machine->a[3]+(int16_t)d(c,3),(uint16_t)value));dw(c,2,(uint16_t)(d(c,2)-1u));
            } while(d(c,2)!=0xffffu);
        }
        c->machine->a[1]+=0x10cu;dw(c,1,(uint16_t)(d(c,1)-1u));
    } while(d(c,1)!=0xffffu);
    memcpy(c->machine->regs->d+1,saved_d,sizeof saved_d);memcpy(c->machine->a+1,saved_a,sizeof saved_a);return WF_OK;
}
wf_status_t wf_r18_match_hud_call(wf_r18_match_hud_t *c,uint32_t owner) {
    if(!c||!c->machine||!c->machine->ram||!c->machine->regs||!c->machine->regs->valid||
       !c->fg_0c0000||c->fg_bytes<0x4000u||(!c->write_palette&&(!c->palette_180000||c->palette_bytes<0x9000u))||!c->read_rom)return WF_BAD_STATE;
    c->machine->dependency=0;
    switch(owner) { case 0x7506u:return hud_7506(c);case 0x782cu:return portrait_782c(c);case 0xc1eau:return palette_c1ea(c);default:return block(c,owner); }
}
