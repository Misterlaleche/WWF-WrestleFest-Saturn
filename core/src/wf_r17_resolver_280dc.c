#include <stdint.h>
#include "wf_r17_resolver_280dc.h"

static int bit(const wf_actor_raw_t *a,size_t o,unsigned b){return (wf_u8(a,o)&(uint8_t)(1u<<b))!=0u;}
static void setbit(wf_actor_raw_t *a,size_t o,unsigned b){wf_set_u8(a,o,(uint8_t)(wf_u8(a,o)|(uint8_t)(1u<<b)));}
static uint32_t udiv32_16(uint32_t n,uint16_t d){
    uint32_t q=0u,r=0u;unsigned i;
    for(i=0u;i<32u;i++){
        r=(r<<1)|((n&0x80000000u)!=0u?1u:0u);n<<=1;q<<=1;
        if(r>=(uint32_t)d){r-=(uint32_t)d;q|=1u;}
    }
    return q;
}
static int divs32_16(int32_t n,uint16_t d,int16_t *out){
    uint32_t mag,q;int neg=n<0;int32_t v;
    if(d==0u||!out)return 0;
    mag=neg?((uint32_t)(-(n+1))+1u):(uint32_t)n;q=udiv32_16(mag,d);
    if((!neg&&q>32767u)||(neg&&q>32768u))return 0;
    v=neg?-(int32_t)q:(int32_t)q;*out=(int16_t)v;return 1;
}
static int d3_left(uint16_t y,int32_t add,int16_t *out){return divs32_16((int32_t)((uint32_t)y<<8)+add,0x02e0u,out);}
static int d3_right(uint16_t y,int32_t sub,int16_t *out){int16_t q;if(!divs32_16((int32_t)((uint32_t)y<<8)-sub,0x02e0u,&q))return 0;*out=(int16_t)(0u-(uint16_t)q);return 1;}
static void corr_y(wf_actor_raw_t *a,uint16_t y,uint16_t bound,uint8_t contact,unsigned flag){
    setbit(a,WF_A_CONTACT+1u,flag);wf_set_be16(a,WF_A_CORR_Y,(uint16_t)(bound-y));wf_set_u8(a,WF_A_CONTACT,contact);
}
static void corr_z(wf_actor_raw_t *a,uint16_t z,uint16_t bound,unsigned flag){
    setbit(a,WF_A_CONTACT+1u,flag);wf_set_be16(a,WF_A_CORR_Z,(uint16_t)((uint16_t)(bound-z)+wf_be16(a,WF_A_FIELD42)));
}
static void corr_x(wf_actor_raw_t *a,uint16_t x,uint16_t edge,uint8_t contact,unsigned flag){
    setbit(a,WF_A_CONTACT+1u,flag);wf_set_be16(a,WF_A_CORR_X,(uint16_t)(edge-x));wf_set_u8(a,WF_A_CONTACT,contact);
}

static wf_status_t route_2818e(wf_actor_raw_t *a,uint16_t x,uint16_t y,uint16_t z){
    int16_t d3;
    if(bit(a,WF_A_FLAGS32,0))return WF_OK;
    if(y<0x0118u)corr_y(a,y,0x0118u,1u,3u);
    if(y>0x0198u)corr_y(a,y,0x0198u,1u,2u);
    if(z<0x0140u){
        corr_z(a,z,0x0140u,4u);
        /* $281EE replaces D2; the later rope-height tests use this WORD. */
        z=wf_be16(a,WF_A_FIELD42);
    }
    if(x<0x0220u){
        if(!d3_left(y,0x00040000,&d3))return WF_BAD_STATE;
        if(x<(uint16_t)d3){setbit(a,WF_A_CONTACT+1u,1u);if(z<0x0180u)corr_x(a,x,(uint16_t)d3,1u,1u);else wf_set_u8(a,WF_A_CONTACT,2u);return WF_OK;}
    }
    if(x>0x02e0u){
        if(!d3_right(y,0x000a3000,&d3))return WF_BAD_STATE;
        if(x>(uint16_t)d3){setbit(a,WF_A_CONTACT+1u,0u);if(z<0x0180u)corr_x(a,x,(uint16_t)d3,1u,0u);else wf_set_u8(a,WF_A_CONTACT,2u);}
    }
    return WF_OK;
}
static wf_status_t route_28288(wf_actor_raw_t *a,uint16_t x,uint16_t y,uint16_t z){
    int16_t d3;
    if(z<0x0100u)corr_z(a,z,0x0100u,4u);
    if(x<0x01c7u){
        if(!d3_left(y,0x00028000,&d3))return WF_BAD_STATE;
        if(x<(uint16_t)d3){corr_x(a,x,(uint16_t)d3,3u,1u);return WF_OK;}
    }
    if(x>0x0360u){
        if(!d3_right(y,0x000ba000,&d3))return WF_BAD_STATE;
        if(x>(uint16_t)d3)corr_x(a,x,(uint16_t)d3,3u,0u);
    }
    return WF_OK;
}
static wf_status_t route_2831c(wf_actor_raw_t *a,uint16_t x,uint16_t y,uint16_t z){
    int16_t d3;uint16_t actual_z=wf_be16(a,WF_A_Z);
    if(bit(a,WF_A_FLAGS32,0))return WF_OK;
    if(y<0x0118u)corr_y(a,y,0x0118u,1u,3u);
    if(y>0x0198u)corr_y(a,y,0x0198u,1u,2u);
    if(z<0x0140u)corr_z(a,z,0x0140u,4u);
    if(actual_z<0x0180u){
        if(x<0x0220u){if(!d3_left(y,0x00040000,&d3))return WF_BAD_STATE;if(x<(uint16_t)d3){corr_x(a,x,(uint16_t)d3,5u,1u);return WF_OK;}}
        if(x>0x02e0u){if(!d3_right(y,0x000a3000,&d3))return WF_BAD_STATE;if(x>(uint16_t)d3)corr_x(a,x,(uint16_t)d3,5u,0u);}
    }else{
        if(x<0x0220u){if(!d3_left(y,0x00038000,&d3))return WF_BAD_STATE;if(x<(uint16_t)d3){corr_x(a,x,(uint16_t)d3,6u,1u);return WF_OK;}}
        if(x>0x02e0u){if(!d3_right(y,0x000ac000,&d3))return WF_BAD_STATE;if(x>(uint16_t)d3)corr_x(a,x,(uint16_t)d3,6u,0u);}
    }
    return WF_OK;
}
static wf_status_t route_28480(wf_actor_raw_t *a,uint16_t x,uint16_t y,uint16_t z){
    if(y!=0x0160u)corr_y(a,y,0x0160u,1u,2u);
    if(z<0x0140u)corr_z(a,z,0x0140u,1u);
    if(x<0x0270u){corr_x(a,x,0x0270u,1u,1u);return WF_OK;}
    if(x>0x03d0u){corr_x(a,x,0x03d0u,1u,0u);return WF_OK;}
    if(x>=0x02b0u&&x<0x0390u)wf_set_u8(a,WF_A_CONTACT,4u);
    return WF_OK;
}
static wf_status_t route_2851e(wf_actor_raw_t *a,uint16_t x,uint16_t y,uint16_t z){
    if(y<0x0110u)corr_y(a,y,0x0110u,1u,3u);
    if(y>0x0140u){
        corr_y(a,y,0x0140u,1u,2u);
        if(x>=0x0294u&&x<0x038cu)wf_set_u8(a,WF_A_CONTACT,4u);
    }
    if(z<0x0100u)corr_z(a,z,0x0100u,4u);
    if(x<0x01c8u){corr_x(a,x,0x01c8u,3u,1u);return WF_OK;}
    if(x>0x0470u)corr_x(a,x,0x0470u,3u,0u);
    return WF_OK;
}

wf_status_t wf_r17_resolver_280dc(wf_runtime_t *rt,wf_actor_raw_t *a){
    uint16_t x,y,z,probe_x;uint8_t mode,side;
    if(!rt||!a)return WF_BAD_STATE;
    wf_set_be16(a,WF_A_CORR_X,0u);wf_set_be16(a,WF_A_CORR_Y,0u);wf_set_be16(a,WF_A_CORR_Z,0u);
    if(bit(a,WF_A_FLAGS32,5u))return WF_OK;
    x=wf_be16(a,WF_A_X);y=(uint16_t)(wf_be16(a,WF_A_Y)+wf_be16(a,WF_A_FIELD40));z=(uint16_t)(wf_be16(a,WF_A_Z)+wf_be16(a,WF_A_FIELD42));
    probe_x=wf_be16(a,WF_A_FIELD3E);if(bit(a,WF_A_FACING,7u)){probe_x=(uint16_t)(0u-probe_x);wf_set_be16(a,WF_A_FIELD3E,probe_x);}x=(uint16_t)(x+probe_x);
    mode=(uint8_t)(rt->g.global_007e&7u);side=(uint8_t)((wf_be16(a,WF_A_FLAGS32)&0x0004u)!=0u);
    if(mode==7u)return WF_BLOCKED;
    if(mode==0u||mode==5u)return side?route_28288(a,x,y,z):route_2818e(a,x,y,z);
    if(mode==1u)return side?WF_OK:route_2831c(a,x,y,z);
    if(mode==2u||mode==6u)return side?route_2851e(a,x,y,z):route_28480(a,x,y,z);
    return WF_OK; /* modes 3/4 dispatch to the exact RTS at $2818C */
}
wf_status_t wf_r17_resolver_280dc_hook(wf_runtime_t *rt,wf_actor_raw_t *actor,void *user){(void)user;return wf_r17_resolver_280dc(rt,actor);}
