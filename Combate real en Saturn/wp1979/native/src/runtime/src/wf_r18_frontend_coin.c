#include "wf_r18_frontend_coin.h"
#include <string.h>
static int valid(const wf_r18_frontend_control_context_t *c) {
    return c&&c->ram_1c0000&&c->ram_bytes>=WF_R18_TITLE_RAM_BYTES&&c->regs&&c->regs->valid&&c->a&&c->effect;
}
static uint16_t r16(const uint8_t *p) { return (uint16_t)(((unsigned)p[0]<<8)|p[1]); }
static void w16(uint8_t *p,uint16_t v) { p[0]=(uint8_t)(v>>8);p[1]=(uint8_t)v; }
static void flags(wf_r18_frontend_control_context_t *c,unsigned value,unsigned bits) {
    c->sr=(uint16_t)((c->sr&0xfff0u)|(value==0u?4u:0u)|(value&(1u<<(bits-1u))?8u:0u));
}
static unsigned arithmetic(wf_r18_frontend_control_context_t *c,unsigned a,unsigned b,unsigned bits,unsigned subtract,unsigned change_x) {
    unsigned mask=(1u<<bits)-1u,sign=1u<<(bits-1u),wide=subtract?a-b:a+b,result=wide&mask;
    unsigned carry=subtract?b>a:wide>mask;
    unsigned overflow=((subtract?(a^b):~(a^b))&(a^result)&sign)!=0u;
    flags(c,result,bits);c->sr=(uint16_t)(c->sr|(overflow?2u:0u)|(carry?1u:0u));
    if(change_x)c->sr=(uint16_t)((c->sr&0xffefu)|(carry?16u:0u));
    return result;
}
static int io(wf_r18_frontend_control_context_t *c,uint32_t kind,uint32_t owner,uint32_t address,uint32_t *value) {
    wf_r18_title_event_t e={kind,owner,address,*value};
    if(c->effect(c->user,&e,c))return WF_R18_CONTROL_BLOCKED;
    *value=e.value;return 0;
}
typedef struct rollback { uint32_t d[3];uint16_t sr;uint8_t ram[6]; } rollback_t;
static void save(wf_r18_frontend_control_context_t *c,rollback_t *b) {
    memcpy(b->d,c->regs->d,sizeof b->d);b->sr=c->sr;memcpy(b->ram,c->ram_1c0000+0x4e,sizeof b->ram);
}
static int restore(wf_r18_frontend_control_context_t *c,const rollback_t *b) {
    memcpy(c->regs->d,b->d,sizeof b->d);c->sr=b->sr;memcpy(c->ram_1c0000+0x4e,b->ram,sizeof b->ram);return WF_R18_CONTROL_BLOCKED;
}
void wf_r18_frontend_coin_enter(wf_r18_frontend_coin_t *s,uint16_t sr,uint32_t pc) {
    if(s){memset(s,0,sizeof *s);s->pc=0x434u;s->return_sr=sr;s->return_pc=pc;s->frame_valid=1;}
}
int wf_r18_frontend_coin_step(wf_r18_frontend_coin_t *s,wf_r18_frontend_control_context_t *c) {
    if(!s||!valid(c)||(s->pc==0x434u&&(!(c->sr&0x2000u)||!s->frame_valid)))return WF_R18_CONTROL_BAD_CONTEXT;
    switch(s->pc) {
    case 0x434u:
        memcpy(s->saved_d,c->regs->d,sizeof s->saved_d);c->ssp-=12u;s->registers_saved=1;s->pc=0x438u;return 0;
    case 0x438u:case 0x480u: {
        uint32_t value=0,address=s->pc==0x438u?0x140016u:0x140002u;
        if(io(c,WF_R18_CONTROL_WRITE16,s->pc,address,&value)) { s->blocked_dependency=s->pc;return WF_R18_CONTROL_BLOCKED; }
        flags(c,0,16u);s->pc=s->pc==0x438u?0x43eu:0x486u;return 0;
    }
    case 0x43eu:
        if(wf_r18_frontend_coin_service_04bc(c)) { s->blocked_dependency=0x4bcu;return WF_R18_CONTROL_BLOCKED; }
        s->pc=0x440u;return 0;
    case 0x440u:case 0x460u: {
        unsigned port=s->pc==0x440u?0u:1u;
        wf_r17_m68k_move_w(c->regs,0u,r16(c->ram_1c0000+(port?0x78u:0x54u)));flags(c,(uint16_t)c->regs->d[0],16u);
        wf_r17_m68k_move_b(c->regs,1u,(uint8_t)port);flags(c,port,8u);
        wf_r17_m68k_move_b(c->regs,2u,c->ram_1c0000[0x10u+port]);flags(c,(uint8_t)c->regs->d[2],8u);
        s->pc=port?0x470u:0x450u;return 0;
    }
    case 0x450u:case 0x470u:
        if(wf_r18_frontend_coin_pulse_048c(c)) { s->blocked_dependency=0x48cu;return WF_R18_CONTROL_BLOCKED; }
        s->pc+=4u;return 0;
    case 0x454u:case 0x474u: {
        unsigned port=s->pc==0x454u?0u:1u;
        w16(c->ram_1c0000+(port?0x78u:0x54u),(uint16_t)c->regs->d[0]);flags(c,(uint16_t)c->regs->d[0],16u);
        c->ram_1c0000[0x10u+port]=(uint8_t)c->regs->d[2];flags(c,(uint8_t)c->regs->d[2],8u);
        s->pc=port?0x480u:0x460u;return 0;
    }
    case 0x486u:
        if(!s->registers_saved)return WF_R18_CONTROL_BAD_CONTEXT;
        memcpy(c->regs->d,s->saved_d,sizeof s->saved_d);c->ssp+=12u;s->registers_saved=0;s->pc=0x48au;return 0;
    case 0x48au:
        if(!s->frame_valid)return WF_R18_CONTROL_BAD_CONTEXT;
        c->sr=(uint16_t)(s->return_sr&0xa71fu);c->ssp+=6u;s->pc=s->return_pc;s->frame_valid=0;return WF_R18_CONTROL_RTE;
    default:s->blocked_dependency=s->pc;return WF_R18_CONTROL_BLOCKED;
    }
}
int wf_r18_frontend_coin_pulse_048c(wf_r18_frontend_control_context_t *c) {
    if(!valid(c))return WF_R18_CONTROL_BAD_CONTEXT;
    uint32_t value=0;
    if(io(c,WF_R18_CONTROL_READ16,0x48cu,0x140020u,&value))return WF_R18_CONTROL_BLOCKED;
    rollback_t before;save(c,&before);
    unsigned inactive=value&(0x100u<<(c->regs->d[1]&7u));
    c->sr=(uint16_t)((c->sr&0xfffbu)|(inactive?0u:4u));
    if(!inactive) {
        uint16_t next=(uint16_t)arithmetic(c,(uint16_t)c->regs->d[0],1u,16u,0,1);
        wf_r17_m68k_move_w(c->regs,0u,next);return 0;
    }
    uint16_t duration=(uint16_t)c->regs->d[0];
    arithmetic(c,duration,1u,16u,1,0);
    if(duration==0u) {
        wf_r17_m68k_move_w(c->regs,0u,0);flags(c,0,16u);
        unsigned release=(uint8_t)c->regs->d[2];arithmetic(c,release,10u,8u,1,0);
        if(release<10u)wf_r17_m68k_move_b(c->regs,2u,(uint8_t)arithmetic(c,release,1u,8u,0,1));
        return 0;
    }
    unsigned release=(uint8_t)c->regs->d[2];arithmetic(c,release,4u,8u,1,0);
    if(release>=4u) {
        arithmetic(c,duration,0x360u,16u,1,0);
        if(duration>=0x360u) { wf_r17_m68k_move_w(c->regs,0u,0);flags(c,0,16u);return 0; }
        if(wf_r18_frontend_coin_credit_04f2(c))return restore(c,&before);
        wf_r17_m68k_move_b(c->regs,2u,0);flags(c,0,8u);
    }
    wf_r17_m68k_move_w(c->regs,0u,0);flags(c,0,16u);return 0;
}
int wf_r18_frontend_coin_service_04bc(wf_r18_frontend_control_context_t *c) {
    if(!valid(c))return WF_R18_CONTROL_BAD_CONTEXT;
    uint32_t value=0;
    if(io(c,WF_R18_CONTROL_READ16,0x4bcu,0x140020u,&value))return WF_R18_CONTROL_BLOCKED;
    c->sr=(uint16_t)((c->sr&0xfffbu)|(value&0x400u?0u:4u));
    uint8_t *ram=c->ram_1c0000;
    if(value&0x400u) {
        arithmetic(c,ram[0x56],30u,8u,1,0);
        if(ram[0x56]>30u)ram[0x4f]=(uint8_t)arithmetic(c,ram[0x4f],1u,8u,0,1);
        ram[0x56]=0;flags(c,0,8u);
    } else {
        flags(c,ram[0x56],8u);
        if(!(ram[0x56]&0x80u))ram[0x56]=(uint8_t)arithmetic(c,ram[0x56],1u,8u,0,1);
    }
    return 0;
}
int wf_r18_frontend_coin_credit_04f2(wf_r18_frontend_control_context_t *c) {
    if(!valid(c))return WF_R18_CONTROL_BAD_CONTEXT;
    uint8_t *ram=c->ram_1c0000;rollback_t before;save(c,&before);
    wf_r17_m68k_move_w(c->regs,1u,0);flags(c,0,16u);
    ram[0x4e]=(uint8_t)arithmetic(c,ram[0x4e],1u,8u,0,1);
    w16(ram+0x52,(uint16_t)arithmetic(c,r16(ram+0x52),1u,16u,0,1));
    uint32_t value=0x312au;
    if(io(c,WF_R18_CONTROL_WRITE16,0x500u,0x14000cu,&value))return restore(c,&before);
    flags(c,0x312au,16u);
    wf_r17_m68k_move_b(c->regs,1u,ram[0x4b]);flags(c,ram[0x4b],8u);
    arithmetic(c,(uint8_t)c->regs->d[1],ram[0x4e],8u,1,0);
    if((uint8_t)c->regs->d[1]>ram[0x4e])return 0;
    wf_r17_m68k_move_b(c->regs,1u,ram[0x4d]);flags(c,ram[0x4d],8u);
    wf_r17_m68k_move_w(c->regs,2u,0);flags(c,0,16u);
    wf_r17_m68k_move_b(c->regs,2u,ram[0x4f]);flags(c,ram[0x4f],8u);
    unsigned sum=arithmetic(c,(uint16_t)c->regs->d[2],(uint16_t)c->regs->d[1],16u,0,1);
    wf_r17_m68k_move_w(c->regs,2u,(uint16_t)sum);arithmetic(c,sum,99u,16u,1,0);
    if(sum>=99u) { ram[0x4f]=99u;flags(c,99u,8u);ram[0x4e]=0;flags(c,0,8u);return 0; }
    ram[0x4f]=(uint8_t)arithmetic(c,ram[0x4f],(uint8_t)c->regs->d[1],8u,0,1);
    w16(ram+0x50,(uint16_t)arithmetic(c,r16(ram+0x50),(uint16_t)c->regs->d[1],16u,0,1));
    wf_r17_m68k_move_b(c->regs,1u,ram[0x4b]);flags(c,ram[0x4b],8u);
    ram[0x4e]=(uint8_t)arithmetic(c,ram[0x4e],(uint8_t)c->regs->d[1],8u,1,1);return 0;
}
