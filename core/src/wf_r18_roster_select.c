#include "wf_r18_roster_select.h"
#include <string.h>

static uint16_t r16(const uint8_t *p){return (uint16_t)(((unsigned)p[0]<<8)|p[1]);}
static void w16(uint8_t *p,uint16_t v){p[0]=(uint8_t)(v>>8);p[1]=(uint8_t)v;}
static void w32(uint8_t *p,uint32_t v){w16(p,(uint16_t)(v>>16));w16(p+2,(uint16_t)v);}
static int valid(wf_r18_roster_select_context_t *c){
    return c&&c->ram_1c0000&&c->ram_bytes>=WF_R18_ROSTER_RAM_BYTES&&c->regs&&c->regs->valid;
}
static void db(wf_r18_roster_select_context_t*c,unsigned n,uint8_t v){wf_r17_m68k_move_b(c->regs,n,v);}
static void dw(wf_r18_roster_select_context_t*c,unsigned n,uint16_t v){wf_r17_m68k_move_w(c->regs,n,v);}
static void dq(wf_r18_roster_select_context_t*c,unsigned n,int8_t v){wf_r17_m68k_moveq(c->regs,n,v);}
static uint16_t d(wf_r18_roster_select_context_t*c,unsigned n){return (uint16_t)c->regs->d[n];}
static int blocked(wf_r18_roster_select_context_t*c,uint32_t pc){c->blocked_dependency=pc;return WF_R18_ROSTER_BLOCKED;}
static int rom(wf_r18_roster_select_context_t*c,uint32_t at,unsigned n,uint32_t *v){
    uint8_t b[4];if(!c->read_rom||at>0x80000u||n>0x80000u-at||c->read_rom(c->user,at,b,n))return blocked(c,at);
    *v=0;for(unsigned i=0;i<n;i++)*v=(*v<<8)|b[i];return WF_R18_ROSTER_OK;
}
static int occupancy(wf_r18_roster_select_context_t*c,unsigned byte_offset,uint8_t **out){
    if(byte_offset>WF_R18_ROSTER_RAM_BYTES-0x11f4u-2u)return blocked(c,0x1c11f4u+byte_offset);
    *out=c->ram_1c0000+0x11f4u+byte_offset;return WF_R18_ROSTER_OK;
}

int wf_r18_roster_scroll_658c(wf_r18_roster_select_context_t*c){
    uint8_t *ram;int st;
    if(!valid(c))return WF_R18_ROSTER_BAD_CONTEXT;
    c->blocked_dependency=0;if(!c->effect)return blocked(c,0x26a42u);
    ram=c->ram_1c0000;dq(c,2,8);dw(c,1,0x210u);
    if(!(ram[0x15e]&2u)){dw(c,2,(uint16_t)-d(c,2));dw(c,1,0x140u);}
    w16(ram+0x17e6u,(uint16_t)(r16(ram+0x17e6u)+d(c,2)));
    st=c->effect(c->user,0x26a42u,c->a,ram,c->regs);
    if(st)return blocked(c,0x26a42u);
    if(d(c,1)==r16(ram+0x17e6u))ram[0x15e]&=(uint8_t)~8u;
    return WF_R18_ROSTER_OK;
}

int wf_r18_roster_reconcile_600e(wf_r18_roster_select_context_t*c){
    uint8_t *ram,*entry;uint32_t v;
    if(!valid(c))return WF_R18_ROSTER_BAD_CONTEXT;
    c->blocked_dependency=0;if(!c->read_rom)return blocked(c,0x62aau);
    ram=c->ram_1c0000;dq(c,0,3);dq(c,2,0);c->a[0]=0x1c05b0u;
    c->a[1]=0x62aau;c->a[2]=0x1c11f4u;
    if(ram[0x15e]&2u)c->a[1]=(ram[0x161]&1u)?0x62b0u:0x62b6u;
    for(unsigned i=0;i<4u;i++){
        uint8_t *a=ram+0x5b0u+i*WF_ACTOR_BYTES;
        if((a[0]&128u)&&!(a[0x33]&1u)){
            db(c,2,a[0x23]);
            if((uint8_t)c->regs->d[2]!=2u&&(uint8_t)c->regs->d[2]!=7u){
                dw(c,2,(uint16_t)(d(c,2)<<1));
                if(occupancy(c,d(c,2),&entry))return WF_R18_ROSTER_BLOCKED;
                w16(entry,0);dq(c,1,5);
                for(;;){
                    if(rom(c,c->a[1]+(int16_t)d(c,1),1,&v))return WF_R18_ROSTER_BLOCKED;
                    db(c,2,(uint8_t)v);dw(c,2,(uint16_t)(d(c,2)<<1));
                    if(occupancy(c,d(c,2),&entry))return WF_R18_ROSTER_BLOCKED;
                    if(!r16(entry)){
                        a[0x23]=(uint8_t)v;a[0x25]=a[0x23];db(c,2,a[0x23]);
                        dw(c,2,(uint16_t)(d(c,2)<<1));w16(entry,1);break;
                    }
                    dw(c,1,(uint16_t)(d(c,1)-1u));if(d(c,1)==0xffffu)break;
                }
            }
        }
        c->a[0]+=WF_ACTOR_BYTES;dw(c,0,(uint16_t)(d(c,0)-1u));
    }
    return WF_R18_ROSTER_OK;
}

int wf_r18_roster_navigate_5b3e(wf_r18_roster_select_context_t*c,unsigned actor_offset){
    uint8_t *ram,*a,*entry;uint32_t value;unsigned index;
    if(!valid(c)||actor_offset>WF_R18_ROSTER_RAM_BYTES-WF_ACTOR_BYTES)return WF_R18_ROSTER_BAD_CONTEXT;
    c->blocked_dependency=0;if(!c->read_rom)return blocked(c,0x62bcu);
    ram=c->ram_1c0000;a=ram+actor_offset;c->a[0]=0x1c0000u+actor_offset;
    c->a[2]=0x62bcu;
    if(ram[0x15e]&2u)c->a[2]=(ram[0x161]&1u)?0x6382u:0x6314u;
    dq(c,0,0);a[0x25]=a[0x23];db(c,0,a[0x23]);
    wf_r17_m68k_move_l(c->regs,0,(uint32_t)d(c,0)*11u);
    dw(c,0,(uint16_t)(d(c,0)+r16(a+0xa8)));
    if(rom(c,c->a[2]+(int16_t)d(c,0),1,&value))return WF_R18_ROSTER_BLOCKED;
    a[0x23]=(uint8_t)value;c->a[3]=0x1c11f4u;dq(c,0,0);db(c,0,a[0x23]);
    dw(c,0,(uint16_t)(d(c,0)<<1));
    if(occupancy(c,d(c,0),&entry))return WF_R18_ROSTER_BLOCKED;
    if(r16(entry)){
        dw(c,0,(uint16_t)(d(c,0)>>1));c->a[3]=0x63f0u;dq(c,1,0);
        db(c,1,a[0x25]);dw(c,1,(uint16_t)(d(c,1)<<2));
        if(rom(c,c->a[3]+(int16_t)d(c,1),4,&value))return WF_R18_ROSTER_BLOCKED;
        c->a[4]=value;
        if(rom(c,c->a[4]+(int16_t)d(c,0),1,&value))return WF_R18_ROSTER_BLOCKED;
        a[0x23]=(uint8_t)value;
        if(!(ram[0x161]&1u)&&a[0x23]==9u)goto restore;
        if((uint8_t)c->regs->d[0]!=a[0x25]){
            db(c,1,a[0x23]);dw(c,1,(uint16_t)(d(c,1)<<1));c->a[3]=0x1c11f4u;
            if(occupancy(c,d(c,1),&entry))return WF_R18_ROSTER_BLOCKED;
            if(r16(entry))goto restore;
        }
        goto reserve;
restore:a[0x23]=a[0x25];
    }
reserve:
    dq(c,0,0);dq(c,1,0);db(c,0,a[0x23]);db(c,1,a[0x25]);
    dw(c,0,(uint16_t)(d(c,0)<<1));dw(c,1,(uint16_t)(d(c,1)<<1));c->a[3]=0x1c11f4u;
    if(occupancy(c,d(c,1),&entry))return WF_R18_ROSTER_BLOCKED;
    w16(entry,0);
    if(occupancy(c,d(c,0),&entry))return WF_R18_ROSTER_BLOCKED;
    w16(entry,1);
    if(ram[0x15e]&8u)return WF_R18_ROSTER_OK;
    dw(c,0,r16(a+0x22));
    if((uint8_t)c->regs->d[0]==a[0x25])return WF_R18_ROSTER_OK;
    if(a[0x23]==2u){if(a[0x25]==7u)return WF_R18_ROSTER_OK;goto request;}
    if(a[0x23]==7u){if(a[0x25]==2u)return WF_R18_ROSTER_OK;goto request;}
    index=(ram[0x15e]&2u)?1u:3u;
    if(a[0x23]==index){if(a[0x25]!=2u)return WF_R18_ROSTER_OK;index=2u;}
    else if(a[0x23]==((ram[0x15e]&2u)?6u:8u)){
        if(a[0x25]!=7u)return WF_R18_ROSTER_OK;
        index=7u;
    }else return WF_R18_ROSTER_OK;
    db(c,1,a[0x23]);db(c,0,a[0x25]);
    dw(c,0,(uint16_t)(d(c,0)<<1));dw(c,1,(uint16_t)(d(c,1)<<1));
    c->a[3]=0x1c11f4u;
    if(occupancy(c,d(c,1),&entry))return WF_R18_ROSTER_BLOCKED;
    w16(entry,0);
    if(occupancy(c,d(c,0),&entry))return WF_R18_ROSTER_BLOCKED;
    w16(entry,1);a[0x23]=(uint8_t)index;
request:
    ram[0x15e]|=9u;return WF_R18_ROSTER_OK;
}

static int emit(wf_r18_roster_select_context_t*c,uint32_t pc){
    if(!c->effect||c->effect(c->user,pc,c->a,c->ram_1c0000,c->regs))return blocked(c,pc);
    return WF_R18_ROSTER_OK;
}
static uint32_t r32(const uint8_t*p){return ((uint32_t)r16(p)<<16)|r16(p+2);}
static uint8_t *actor_at(wf_r18_roster_select_context_t*c,uint32_t address){
    if(address<0x1c0000u||address-0x1c0000u>c->ram_bytes-WF_ACTOR_BYTES){blocked(c,address);return 0;}
    return c->ram_1c0000+address-0x1c0000u;
}
static int port_at(wf_r18_roster_select_context_t*c,uint32_t address,uint16_t *value){
    if(address<0x140020u||address>0x140026u||(address&1u))return blocked(c,address);
    *value=c->input_words[(address-0x140020u)/2u];return 0;
}
/* $64E0's public owner intentionally excludes A3; retain its exact address
 * side effects here, where the original raw input pointer is available. */
static int input_64e0(wf_r18_roster_select_context_t*c,uint8_t*a){
    wf_actor_raw_t actor;wf_r18_frontend_input_context_t input;uint16_t port=0xffffu;
    uint8_t *ram=c->ram_1c0000;uint16_t terminal=(uint16_t)(r16(a+0x5e)+1u);
    uint16_t timer=(uint16_t)(r16(a+0x56)+1u);uint32_t rng=r32(ram+0x5c);
    if(!(ram[0x15e]&16u)){
        c->a[3]=r32(a+0x8a);if(port_at(c,c->a[3],&port))return WF_R18_ROSTER_BLOCKED;
    }
    memcpy(&actor,a,sizeof actor);input.rng_state_1c005c=&rng;
    input.frame_1c0080=r32(ram+0x80);input.flags_1c015e=ram[0x15e];
    if(wf_r18_frontend_input_64e0(&actor,port,&input,c->regs))return blocked(c,0x64e0u);
    memcpy(a,&actor,sizeof actor);w32(ram+0x5c,rng);
    if((ram[0x15e]&16u)&&terminal<64u&&timer&&r16(a+0x56)==0)c->a[3]=0x6584u;
    return 0;
}
static int activate_5f28(wf_r18_roster_select_context_t*c){
    uint8_t *ram=c->ram_1c0000,*a=actor_at(c,c->a[1]),*partner,*entry;uint32_t v;
    if(!a)return WF_R18_ROSTER_BLOCKED;
    w16(a,0x8000u);w16(a+2,0xdu);c->a[3]=0x6484u;
    dw(c,0,(uint16_t)(r16(ram+0x66)&0x800u));
    if(d(c,0)==0x800u&&!(ram[0x161]&1u)&&!(ram[0x160]&128u))c->a[3]=0x6494u;
    dw(c,3,(uint16_t)(d(c,3)<<2));
    if(rom(c,c->a[3]+(int16_t)d(c,3),2,&v))return WF_R18_ROSTER_BLOCKED;
    w16(a+4,(uint16_t)v);
    if(rom(c,c->a[3]+(int16_t)d(c,3)+2u,2,&v))return WF_R18_ROSTER_BLOCKED;
    w16(a+0xae,(uint16_t)v);w16(a+14,0x140u);w16(a+0x58,0);w16(a+0x5c,0);
    dw(c,0,r16(a+2));if(emit(c,0x2aeau))return WF_R18_ROSTER_BLOCKED;
    if(!(ram[0x161]&1u)){
        c->a[3]=r32(a+0x86);partner=actor_at(c,c->a[3]);if(!partner)return WF_R18_ROSTER_BLOCKED;
        if((partner[0]&128u)&&(partner[0x33]&1u)){
            if(partner[0x23]==8u){
                a[0x33]|=1u;a[0x23]=partner[0x23];a[0x23]++;
                a[0x23]=partner[0x25]; /* Original $5FC0 writes +23 again. */
            }
            return 0;
        }
    }
    dq(c,0,5);dq(c,1,0);c->a[2]=0x1c11f4u;c->a[3]=(ram[0x15e]&2u)?0x62b0u:0x62aau;
    for(;;){
        if(rom(c,c->a[3]+(int16_t)d(c,0),1,&v))return WF_R18_ROSTER_BLOCKED;
        db(c,1,(uint8_t)v);dw(c,1,(uint16_t)(d(c,1)<<1));
        if(occupancy(c,d(c,1),&entry))return WF_R18_ROSTER_BLOCKED;
        if(!r16(entry))break;
        dw(c,0,(uint16_t)(d(c,0)-1u));if(d(c,0)==0xffffu)break;
    }
    w16(entry,1);dw(c,1,(uint16_t)(d(c,1)>>1));a[0x23]=(uint8_t)d(c,1);a[0x25]=(uint8_t)d(c,1);w16(a+0x32,0);
    return 0;
}
/* Selector calls $055A with MOVEQ #0,D0 only. Its original $05CE mode
 * spends one whole credit and changes neither D nor A registers. */
static int payment_mode0(wf_r18_roster_select_context_t*c){
    uint8_t *credit=c->ram_1c0000+0x4f;if(!*credit)return 0;--*credit;return 1;
}
static int input_7304(wf_r18_roster_select_context_t*c){
    uint8_t *ram=c->ram_1c0000;uint16_t port;
    c->a[3]=r32(ram+0x156);if(port_at(c,c->a[3],&port))return WF_R18_ROSTER_BLOCKED;
    dw(c,3,(uint16_t)((uint16_t)~port&0xffu));ram[0x15e3]=(uint8_t)d(c,3)&15u;
    db(c,4,ram[0x15e2]);
    if((uint8_t)d(c,4)==ram[0x15e3])ram[0x15e3]=0;else ram[0x15e2]=ram[0x15e3];
    db(c,3,(uint8_t)(((uint8_t)d(c,3)>>4)&3u));
    ram[0x15e0]&=(uint8_t)d(c,3);ram[0x15e0]^=(uint8_t)d(c,3);
    ram[0x15e1]=ram[0x15e0];ram[0x15e0]=(uint8_t)d(c,3);return 0;
}
/* carry_out is the original CCR.C at $6108/$61A4/$6174. All other
 * state remains the live original D/A/raw-RAM state passed to $5F28. */
static int joins_60a2(wf_r18_roster_select_context_t*c,int *carry_out){
    uint8_t *ram=c->ram_1c0000,*a;uint32_t v;uint16_t port;
    *carry_out=0;dw(c,0,(uint16_t)(r16(ram+0x66)&0x800u));
    if(d(c,0)!=0x800u){
        dq(c,1,3);dq(c,3,0);c->a[0]=0x140020u;c->a[1]=0x1c05b0u;
        for(;;){
            if(port_at(c,c->a[0],&port))return WF_R18_ROSTER_BLOCKED;
            a=actor_at(c,c->a[1]);if(!a)return WF_R18_ROSTER_BLOCKED;
            if(!(port&128u)&&!(a[0xa1]&1u)){
                dq(c,0,0);if(payment_mode0(c)){
                    w32(a+0x8a,c->a[0]);a[0xa1]|=1u;w16(ram+0x15c,0);*carry_out=1;return 0;
                }
            }
            c->a[0]+=2u;db(c,3,(uint8_t)(d(c,3)+1u));c->a[1]+=WF_ACTOR_BYTES;
            dw(c,1,(uint16_t)(d(c,1)-1u));if(d(c,1)==0xffffu)return 0;
        }
    }
    if(ram[0x15e]&16u)return 0;
    if(!(ram[0x15e]&32u)){
        dw(c,0,(uint16_t)(c->input_words[1]&128u));if(d(c,0))return 0;
        dq(c,0,0);if(!payment_mode0(c))return 0;
        if(ram[0x161]&1u){
            c->a[1]=0x1c06bcu;dq(c,3,1);a=ram+0x6bcu;a[0xa1]|=1u;w32(a+0x8a,0x140022u);
            ram[0x160]|=128u;ram[0x15e]|=96u;*carry_out=1;return 0;
        }
        ram[0x15e]|=32u;dq(c,0,0);if(emit(c,0x51d2u))return WF_R18_ROSTER_BLOCKED;
        dq(c,0,1);if(emit(c,0x51d2u))return WF_R18_ROSTER_BLOCKED;
        ram[0x160]|=64u;w32(ram+0x156,0x140022u);return 0;
    }
    if(ram[0x15e]&64u)return 0;
    w16(ram+0x15c,0);w16(ram+0x1698,(uint16_t)(r16(ram+0x1698)+1u));
    if(r16(ram+0x1698)<0x200u){
        if(input_7304(c))return WF_R18_ROSTER_BLOCKED;
        if(!ram[0x15e1]){
            dq(c,0,0);db(c,0,ram[0x15e3]);c->a[2]=(ram[0x160]&64u)?0x54f8u:0x5503u;
            if(rom(c,c->a[2]+(int16_t)d(c,0),1,&v))return WF_R18_ROSTER_BLOCKED;
            if((v!=0u)==((ram[0x160]&64u)!=0u)){
                dq(c,0,1);if(emit(c,0x527eu))return WF_R18_ROSTER_BLOCKED;
                dq(c,0,2);if(emit(c,0x51d2u))return WF_R18_ROSTER_BLOCKED;
                ram[0x160]|=128u;ram[0x160]&=(uint8_t)~64u;
            }else{
                dq(c,0,2);if(emit(c,0x527eu))return WF_R18_ROSTER_BLOCKED;
                dq(c,0,1);if(emit(c,0x51d2u))return WF_R18_ROSTER_BLOCKED;
                ram[0x160]|=64u;ram[0x160]&=(uint8_t)~128u;
            }
            return 0;
        }
    }
    for(int i=0;i<3;i++){dq(c,0,(int8_t)i);if(emit(c,0x527eu))return WF_R18_ROSTER_BLOCKED;}
    if(ram[0x160]&64u){c->a[1]=0x1c07c8u;a=ram+0x7c8u;a[0xa7]=1;dq(c,3,2);}
    else{c->a[1]=0x1c06bcu;a=ram+0x6bcu;dq(c,3,1);}
    a[0xa1]|=1u;w32(a+0x8a,0x140022u);ram[0x15e]|=64u;*carry_out=1;return 0;
}
static int palette_5eae(wf_r18_roster_select_context_t*c,uint8_t*a){
    uint32_t v,pc;c->a[1]=0x188000u;dq(c,3,0);db(c,3,a[0x23]);
    c->a[3]=(c->ram_1c0000[0x161]&1u)?0x5f1eu:0x5f14u;
    if(rom(c,c->a[3]+(int16_t)d(c,3),1,&v))return WF_R18_ROSTER_BLOCKED;
    db(c,3,(uint8_t)v);dw(c,3,(uint16_t)(d(c,3)<<7));c->a[2]=0x5f0au;
    dw(c,2,r16(a+0x5c));dw(c,2,(uint16_t)(d(c,2)<<1));
    if(a[0x59]==0x4bu){v=r16(a+0xae);pc=0x5eeau;}
    else{pc=0x5ef2u;if(rom(c,c->a[2]+(int16_t)d(c,2),2,&v))return WF_R18_ROSTER_BLOCKED;}
    if(!c->write_palette||c->write_palette(c->user,pc,c->a[1]+(int16_t)d(c,3)+0x1eu,(uint16_t)v))return blocked(c,pc);
    w16(a+0x5c,(uint16_t)(r16(a+0x5c)+1u));if(a[0x5d]==5u)w16(a+0x5c,0);return 0;
}
static int geometry_5cd0(wf_r18_roster_select_context_t*c,uint8_t*a){
    uint32_t v;c->a[2]=0x64a4u;dq(c,0,0);db(c,0,a[0x23]);dw(c,0,(uint16_t)(d(c,0)<<2));
    if(rom(c,c->a[2]+(int16_t)d(c,0),2,&v))return WF_R18_ROSTER_BLOCKED;
    w16(a+6,(uint16_t)v);
    if(rom(c,c->a[2]+(int16_t)d(c,0)+2u,2,&v))return WF_R18_ROSTER_BLOCKED;
    w16(a+10,(uint16_t)v);if(c->ram_1c0000[0x161]&1u)w16(a+10,(uint16_t)(r16(a+10)+0x200u));
    if(emit(c,0x247cu)||emit(c,0x27b8u))return WF_R18_ROSTER_BLOCKED;
    return 0;
}
static int live_actor_59d2(wf_r18_roster_select_context_t*c){
    uint8_t *ram=c->ram_1c0000,*a=actor_at(c,c->a[0]),*p,*entry;uint32_t v;
    if(!a)return WF_R18_ROSTER_BLOCKED;
    if(!(a[0]&128u)||(a[0x33]&1u))return 0;
    if(ram[0x15e]&8u)return geometry_5cd0(c,a);
    if(input_64e0(c,a))return WF_R18_ROSTER_BLOCKED;
    if(!a[0xa3]){
        if(wf_r18_roster_navigate_5b3e(c,(unsigned)(a-ram)))return WF_R18_ROSTER_BLOCKED;
        return geometry_5cd0(c,a);
    }
    c->a[3]=0x1c11f4u;dq(c,0,0);db(c,0,a[0x23]);dw(c,0,(uint16_t)(d(c,0)<<1));
    if(occupancy(c,d(c,0)+0x20u,&entry))return WF_R18_ROSTER_BLOCKED;
    if(r16(entry))goto rejected;
    w16(entry,1);if(occupancy(c,d(c,0),&entry))return WF_R18_ROSTER_BLOCKED;w16(entry,0);
    if(ram[0x161]&1u){a[0x33]|=1u;return 0;}
    if(a[0x23]==8u){
        c->a[2]=r32(a+0x86);p=actor_at(c,c->a[2]);if(!p)return WF_R18_ROSTER_BLOCKED;
        if(!(p[0]&128u))w32(p+0x8a,r32(a+0x8a));
        if(p[0x33]&1u)goto rejected;
        p[0x33]|=1u;c->a[3]=0x1c11f4u;db(c,0,p[0x23]);dw(c,0,(uint16_t)(d(c,0)<<1));
        if(occupancy(c,d(c,0),&entry))return WF_R18_ROSTER_BLOCKED;
        w16(entry,0);
        p[0]|=128u;p[0x33]|=1u;p[0x23]=a[0x23];p[0x25]=a[0x25];w16(p+0xae,r16(a+0xae));
        if(c->a[2]<c->a[0])a[0x23]++;else p[0x23]++;
        a[0x33]|=1u;return 0;
    }
    a[0x33]|=1u;c->a[1]=r32(a+0x86);p=actor_at(c,c->a[1]);if(!p)return WF_R18_ROSTER_BLOCKED;
    if(p[0]&128u)return 0;
    if(p[0xa1]&1u){
        if(!(a[0xa1]&1u)&&((uint8_t)d(c,6)==3u||(uint8_t)d(c,6)==1u)){
            dw(c,1,r16(a+0x22));dw(c,2,r16(p+0x22));w16(p+0x22,d(c,1));w16(a+0x22,d(c,2));
        }
        return 0;
    }
    w16(p+0xa8,r16(a+0xa8));w16(p+0x90,r16(a+0x90));w16(p+0xa2,r16(a+0xa2));
    w16(p+0xa6,r16(a+0xa6));w32(p+0x8a,r32(a+0x8a));c->a[3]=0x62a6u;dq(c,3,0);
    if(rom(c,c->a[3]+(int16_t)d(c,6),1,&v))return WF_R18_ROSTER_BLOCKED;
    db(c,3,(uint8_t)v);
    if(activate_5f28(c))return WF_R18_ROSTER_BLOCKED;
    p[0x23]=a[0x23];p[0x25]=a[0x25];
    return geometry_5cd0(c,a);
rejected:
    if(ram[0x15e]&16u)w16(a+0xa2,0);
    return geometry_5cd0(c,a);
}
void wf_r18_roster_select_init(wf_r18_roster_select_t*s){if(s)s->pc=0x57e0u;}
int wf_r18_roster_select_step(wf_r18_roster_select_t*s,wf_r18_roster_select_context_t*c){
    uint8_t *ram,*a;uint32_t value;unsigned offset;
    if(!s||!valid(c))return WF_R18_ROSTER_BAD_CONTEXT;
    c->blocked_dependency=0;ram=c->ram_1c0000;
    switch(s->pc){
    case 0x57e0u:
        dq(c,2,3);dq(c,3,0);dq(c,1,0);c->a[0]=0x1c05b0u;
        c->a[2]=0x646cu;c->a[3]=0x647cu;c->a[4]=0x6484u;
        dw(c,0,(uint16_t)(r16(ram+0x66u)&0x800u));
        if(d(c,0)==0x800u){
            if(ram[0x161]&1u){
                if(ram[0x160]&64u){
                    w16(ram+0x7c8u,0);w32(ram+0x852u,0);
                    w16(ram+0x6bcu,0x8000u);w32(ram+0x746u,0x140022u);
                }
            }else if(!(ram[0x160]&128u))c->a[4]=0x6494u;
        }
        s->pc=0x5852u;return WF_R18_ROSTER_OK;
    case 0x5852u:
        if(c->a[0]<0x1c0000u||c->a[0]-0x1c0000u>WF_R18_ROSTER_RAM_BYTES-WF_ACTOR_BYTES)return blocked(c,0x5852u);
        a=ram+c->a[0]-0x1c0000u;
        if(a[0]&128u){
            w16(a+2,0xdu);dw(c,1,d(c,3));dw(c,1,(uint16_t)(d(c,1)<<1));
            if(rom(c,c->a[3]+(int16_t)d(c,1),2,&value))return WF_R18_ROSTER_BLOCKED;
            w16(a+0x22,(uint16_t)value);dw(c,1,(uint16_t)(d(c,1)<<1));
            if(rom(c,c->a[2]+(int16_t)d(c,1),2,&value))return WF_R18_ROSTER_BLOCKED;
            w16(a+6,(uint16_t)value);
            if(rom(c,c->a[2]+(int16_t)d(c,1)+2u,2,&value))return WF_R18_ROSTER_BLOCKED;
            w16(a+10,(uint16_t)value);w16(a+14,0x140u);
            if(rom(c,c->a[4]+(int16_t)d(c,1),2,&value))return WF_R18_ROSTER_BLOCKED;
            w16(a+4,(uint16_t)value);
            if(rom(c,c->a[4]+(int16_t)d(c,1)+2u,2,&value))return WF_R18_ROSTER_BLOCKED;
            w16(a+0xae,(uint16_t)value);w16(a+0x32,0);a[0xa1]|=1u;
            w16(a+0x58,0);w16(a+0x5c,0);dw(c,0,r16(a+2));
            if(emit(c,0x2aeau))return WF_R18_ROSTER_BLOCKED;
        }
        dw(c,3,(uint16_t)(d(c,3)+1u));c->a[0]+=WF_ACTOR_BYTES;dw(c,2,(uint16_t)(d(c,2)-1u));
        if(d(c,2)!=0xffffu)return WF_R18_ROSTER_OK;
        if(!(ram[0x161]&1u)){
            w32(ram+0x742u,0x1c05b0u);w32(ram+0x636u,0x1c06bcu);
            w32(ram+0x95au,0x1c07c8u);w32(ram+0x84eu,0x1c08d4u);
            s->pc=0x58e4u;
        }else s->pc=0x592au;
        return WF_R18_ROSTER_OK;
    case 0x58e4u:
    case 0x592au:
        if(!(c->input_words[3]&0x400u))return WF_R18_ROSTER_WAIT;
        w16(ram+0x1804u,0x140u);w16(ram+0x1806u,s->pc==0x58e4u?0x300u:0x500u);
        w16(ram+0x7eu,3);
        if(emit(c,0x26e66u)||emit(c,0x1f9eu))return WF_R18_ROSTER_BLOCKED;
        w16(ram+0x15f4u,3);w16(ram+0x15f8u,3);
        if(emit(c,0x2a06u))return WF_R18_ROSTER_BLOCKED;
        s->pc=0x596eu;return WF_R18_ROSTER_OK;
    case 0x596eu:
        if(c->input_words[3]&0x400u)return WF_R18_ROSTER_WAIT;
        dw(c,0,0x10u);if(emit(c,0x21e6u))return WF_R18_ROSTER_BLOCKED;
        dq(c,0,9);c->a[0]=0x1c0598u;
        for(offset=0;offset<20u;offset+=2){w16(ram+0x598u+offset,0xffffu);c->a[0]+=2u;dw(c,0,(uint16_t)(d(c,0)-1u));}
        s->pc=0x5990u;return WF_R18_ROSTER_LIVE;
    case 0x5990u:
        w16(ram+0x15c,0);
        __attribute__((fallthrough));
    case 0x5996u:
        w32(ram+0x80,r32(ram+0x80)+1u);w16(ram+0x6e,0);
        if(ram[0x15e]&8u){if(wf_r18_roster_scroll_658c(c))return WF_R18_ROSTER_BLOCKED;}
        else{w16(ram+0x15c,(uint16_t)(r16(ram+0x15c)+1u));if(r16(ram+0x15c)>=0x400u)ram[0x15e]|=16u;}
        c->a[0]=0x1c05b0u;dq(c,6,3);s->pc=0x59d2u;return WF_R18_ROSTER_OK;
    case 0x59d2u:
        if(live_actor_59d2(c))return WF_R18_ROSTER_BLOCKED;
        c->a[0]+=WF_ACTOR_BYTES;dw(c,6,(uint16_t)(d(c,6)-1u));
        if(d(c,6)==0xffffu)s->pc=0x5d10u;
        return WF_R18_ROSTER_OK;
    case 0x5d10u:
        if(emit(c,0x2836u))return WF_R18_ROSTER_BLOCKED;
        ram[0x6f]|=1u;s->pc=0x5d1eu;return WF_R18_ROSTER_OK;
    case 0x5d1eu:
        if(!(ram[0x6f]&2u))return WF_R18_ROSTER_WAIT;
        s->pc=0x5d28u;return WF_R18_ROSTER_OK;
    case 0x5d28u:{
        int carry;if(joins_60a2(c,&carry))return WF_R18_ROSTER_BLOCKED;
        if(carry&&activate_5f28(c))return WF_R18_ROSTER_BLOCKED;
        s->pc=0x5d32u;return WF_R18_ROSTER_OK;
    }
    case 0x5d32u:
        if(ram[0x15e]&1u){
            ram[0x15e]&=(uint8_t)~1u;ram[0x15e]^=2u;
            if(wf_r18_roster_reconcile_600e(c))return WF_R18_ROSTER_BLOCKED;
        }
        s->pc=0x5d48u;return WF_R18_ROSTER_OK;
    case 0x5d48u:
        dq(c,5,0);dq(c,4,3);c->a[0]=0x1c05b0u;s->pc=0x5d52u;return WF_R18_ROSTER_OK;
    case 0x5d52u:
        a=actor_at(c,c->a[0]);if(!a)return WF_R18_ROSTER_BLOCKED;
        if(a[0]&128u){
            if(!(a[0x33]&1u))dq(c,5,1);
            else{
                w16(a+0x58,(uint16_t)(r16(a+0x58)+1u));
                if(r16(a+0x58)<0x4cu){if(palette_5eae(c,a))return WF_R18_ROSTER_BLOCKED;dq(c,5,1);}
            }
        }
        c->a[0]+=WF_ACTOR_BYTES;dw(c,4,(uint16_t)(d(c,4)-1u));
        if(d(c,4)==0xffffu)s->pc=0x5d82u;
        return WF_R18_ROSTER_OK;
    case 0x5d82u:
        s->pc=(c->regs->d[5]||((ram[0x15e]&32u)&&!(ram[0x15e]&64u)))?0x5996u:0x5da0u;
        return WF_R18_ROSTER_OK;
    case 0x5da0u:
        c->a[1]=0x1c0598u;c->a[0]=0x1c05b0u;c->a[2]=0x64ccu;dq(c,0,3);dq(c,1,0);dq(c,2,0);
        for(offset=0;offset<4u;offset++){
            a=ram+0x5b0u+offset*WF_ACTOR_BYTES;
            if(a[0]&128u){
                dw(c,1,r16(a+0x22));dw(c,2,d(c,1));dw(c,2,(uint16_t)(d(c,2)<<1));
                if(rom(c,c->a[2]+(int16_t)d(c,2),2,&value))return WF_R18_ROSTER_BLOCKED;
                w16(ram+c->a[1]-0x1c0000u,(uint16_t)value);c->a[1]+=2u;
                w16(a+0x56,(uint16_t)value);w16(a+2,r16(a+0x56));
            }
            dw(c,1,(uint16_t)(d(c,1)+1u));c->a[0]+=WF_ACTOR_BYTES;dw(c,0,(uint16_t)(d(c,0)-1u));
        }
        if(!(ram[0x161]&1u)){
            c->a[0]=0x1c05b0u;c->a[1]=0x1c06bcu;c->a[3]=0x1c07c8u;c->a[4]=0x1c08d4u;c->a[5]=0x1c0598u;
            a=ram+0x5b0u;
            if((a[0]&128u)&&!(a[0xa1]&1u)){
                dw(c,0,r16(a+0x56));w16(a+0x56,r16(ram+0x6bcu+0x56));w16(ram+0x6bcu+0x56,d(c,0));
                dw(c,0,r16(a+2));w16(a+2,r16(ram+0x6bcu+2));w16(ram+0x6bcu+2,d(c,0));
                dw(c,0,r16(ram+0x598));w16(ram+0x598,r16(ram+0x59a));w16(ram+0x59a,d(c,0));
            }
            /* $5E48 still tests A2=$64CC, the original mapping-table ROM
             * address. Do not "repair" this into an actor2 pointer. */
            if(rom(c,c->a[2],1,&value))return WF_R18_ROSTER_BLOCKED;
            if(value&128u)return blocked(c,0x5e50u);
        }
        s->pc=0x5e82u;return WF_R18_ROSTER_OK;
    case 0x5e82u:
        w32(ram+0x156,0);w32(ram+0x15a,0);w16(ram+0x15c,0);w16(ram+0x15e,0);w16(ram+0x1698,0);
        s->pc=0x5ea0u;return WF_R18_ROSTER_OK;
    case 0x5ea0u:
        dw(c,0,0x20u);if(emit(c,0x21e6u))return WF_R18_ROSTER_BLOCKED;
        s->pc=0xac0u;return WF_R18_ROSTER_COMBAT;
    case 0xac0u:return WF_R18_ROSTER_COMBAT;
    default:return blocked(c,s->pc);
    }
}
