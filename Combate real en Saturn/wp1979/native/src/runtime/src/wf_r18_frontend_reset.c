#include "wf_r18_frontend_reset.h"
#include <string.h>
static int valid(const wf_r18_frontend_control_context_t *c) {
    return c&&c->ram_1c0000&&c->ram_bytes>=WF_R18_TITLE_RAM_BYTES&&c->regs&&c->regs->valid&&c->a;
}
static void flags(wf_r18_frontend_control_context_t *c,uint16_t value) {
    c->sr=(uint16_t)((c->sr&0xfff0u)|(value==0u?4u:0u)|(value&0x8000u?8u:0u));
}
static uint16_t r16(const uint8_t *p) { return (uint16_t)(((unsigned)p[0]<<8)|p[1]); }
static void w16(uint8_t *p,uint16_t value) { p[0]=(uint8_t)(value>>8);p[1]=(uint8_t)value; }
static int read16(wf_r18_frontend_control_context_t *c,uint32_t owner,uint32_t address,uint16_t *value) {
    wf_r18_title_event_t e={WF_R18_CONTROL_READ16,owner,address,0};
    if(c->effect(c->user,&e,c))return WF_R18_CONTROL_BLOCKED;
    *value=(uint16_t)e.value;return 0;
}
static int emit(wf_r18_frontend_reset_t *s,wf_r18_frontend_control_context_t *c,
    uint32_t kind,uint32_t owner,uint32_t address,uint32_t value) {
    wf_r18_title_event_t e={kind,owner,address,value};
    if(c->effect(c->user,&e,c)) { s->blocked_dependency=owner;return WF_R18_CONTROL_BLOCKED; }
    return 0;
}
void wf_r18_frontend_reset_init(wf_r18_frontend_reset_t *s) { if(s){s->pc=0x654u;s->blocked_dependency=0;} }
int wf_r18_frontend_reset_step(wf_r18_frontend_reset_t *s,wf_r18_frontend_control_context_t *c) {
    if(!s||!valid(c)||!c->effect||(s->pc==0x654u&&!(c->sr&0x2000u)))return WF_R18_CONTROL_BAD_CONTEXT;
    switch(s->pc) {
    case 0x654u:
        c->sr=(uint16_t)((c->sr|0x700u)&0xa71fu);c->usp=0x1c3fd4u;
        memset(c->regs->d,0,sizeof c->regs->d);memset(c->a,0,7u*sizeof *c->a);flags(c,0);
        s->pc=0x69eu;return 0;
    case 0x69eu:
        if(emit(s,c,WF_R18_CONTROL_WRITE16,0x69eu,0x184000u,0))return WF_R18_CONTROL_BLOCKED;
        flags(c,0);s->pc=0x6a6u;return 0;
    case 0x6a6u:
        w16(c->ram_1c0000+0x76,0);w16(c->ram_1c0000+0x7a,0xffffu);w16(c->ram_1c0000+0x1680,0);
        flags(c,0);s->pc=0x6beu;return 0;
    case 0x6beu:case 0x6deu:
        if(emit(s,c,WF_R18_CONTROL_CALL,0x1fc0u,0,0))return WF_R18_CONTROL_BLOCKED;
        s->pc+=4u;return 0;
    case 0x6c2u:
        if(emit(s,c,WF_R18_CONTROL_WRITE16,0x6c2u,0x14000cu,0x3100u))return WF_R18_CONTROL_BLOCKED;
        flags(c,0x3100u);s->pc=0x6cau;return 0;
    case 0x6cau: {
        uint16_t captured=(uint16_t)(c->sr&0xd9ffu);
        wf_r17_m68k_move_w(c->regs,0u,captured);c->sr=(uint16_t)(captured&0xa71fu);s->pc=0x6d2u;return 0;
    }
    case 0x6d2u:
        wf_r18_frontend_reset_clear_1f1e(c);s->pc=0x6d6u;return 0;
    case 0x6d6u:
        if(wf_r18_frontend_reset_dips_1e1e(c)) { s->blocked_dependency=0x1e1eu;return WF_R18_CONTROL_BLOCKED; }
        s->pc=0x6dau;return 0;
    case 0x6dau:
        if(emit(s,c,WF_R18_CONTROL_CALL,0x2006u,0,0))return WF_R18_CONTROL_BLOCKED;
        s->pc=0x6deu;return 0;
    case 0x6e2u:
        wf_r18_frontend_reset_coinage_0400(c);s->pc=0x6e6u;return 0;
    case 0x6e6u:case 0x6eeu: {
        uint32_t address=s->pc==0x6e6u?0x140002u:0x140000u;
        if(emit(s,c,WF_R18_CONTROL_WRITE16,s->pc,address,0))return WF_R18_CONTROL_BLOCKED;
        flags(c,0);s->pc+=8u;return 0;
    }
    case 0x6f6u:
        wf_r18_frontend_reset_rankings_6d82(c);s->pc=0x6fcu;return WF_R18_CONTROL_HANDOFF;
    case 0x6fcu:return WF_R18_CONTROL_HANDOFF;
    default:s->blocked_dependency=s->pc;return WF_R18_CONTROL_BLOCKED;
    }
}
int wf_r18_frontend_reset_clear_1f1e(wf_r18_frontend_control_context_t *c) {
    if(!valid(c))return WF_R18_CONTROL_BAD_CONTEXT;
    memset(c->ram_1c0000+0x60,0,0x3ad4u-0x60u);flags(c,0);return 0;
}
int wf_r18_frontend_reset_dips_1e1e(wf_r18_frontend_control_context_t *c) {
    if(!valid(c)||!c->effect)return WF_R18_CONTROL_BAD_CONTEXT;
    uint32_t saved_d0=c->regs->d[0];uint16_t saved_sr=c->sr,saved_config=r16(c->ram_1c0000+0x66),value;
    static const uint32_t pcs[4]={0x1e1eu,0x1e30u,0x1e40u,0x1e52u};
    for(unsigned port=0;port<4u;port++) {
        if(read16(c,pcs[port],0x140020u+port*2u,&value)) {
            c->regs->d[0]=saved_d0;c->sr=saved_sr;w16(c->ram_1c0000+0x66,saved_config);
            return WF_R18_CONTROL_BLOCKED;
        }
        if(port==0u) { value=(uint16_t)((value&0x3000u)<<2);c->sr&=0xffefu; }
        else if(port==1u)value&=0x3f00u;
        else if(port==2u) { value=(uint16_t)((value&0x3f00u)>>8);c->sr&=0xffefu; }
        else { value=(uint16_t)((value&0x300u)>>2);c->sr&=0xffefu; }
        wf_r17_m68k_move_w(c->regs,0u,value);
        uint16_t config=port==0u?value:(uint16_t)(r16(c->ram_1c0000+0x66)|value);
        w16(c->ram_1c0000+0x66,config);flags(c,config);
    }
    value=(uint16_t)~r16(c->ram_1c0000+0x66);w16(c->ram_1c0000+0x66,value);flags(c,value);return 0;
}
int wf_r18_frontend_reset_coinage_0400(wf_r18_frontend_control_context_t *c) {
    static const uint8_t table_042c[8]={1u,1u,1u,2u,2u,1u,3u,1u};
    if(!valid(c))return WF_R18_CONTROL_BAD_CONTEXT;
    uint16_t index=(uint16_t)((c->ram_1c0000[0x67]&3u)*2u);
    c->a[0]=0x42cu;wf_r17_m68k_move_w(c->regs,0u,index);
    c->ram_1c0000[0x4b]=table_042c[index];c->ram_1c0000[0x4d]=table_042c[index+1u];
    w16(c->ram_1c0000+0x4e,0);c->sr&=0xffefu;flags(c,0);return 0;
}
int wf_r18_frontend_reset_rankings_6d82(wf_r18_frontend_control_context_t *c) {
    /* Exact adjacent 66-byte factory tables at ROM$6DB0/$6DF2. */
    static const uint8_t table_6db0[132]={
        0x08u,0x54u,0x41u,0x4bu,0x00u,0x01u,0x06u,0x48u,0x41u,0x47u,0x02u,0x03u,
        0x05u,0x48u,0x41u,0x4eu,0x06u,0x07u,0x05u,0x53u,0x49u,0x4eu,0x08u,0x09u,
        0x04u,0x54u,0x41u,0x54u,0x02u,0x03u,0x04u,0x4bu,0x49u,0x4eu,0x00u,0x01u,
        0x03u,0x4eu,0x41u,0x52u,0x02u,0x03u,0x03u,0x4du,0x41u,0x45u,0x06u,0x07u,
        0x02u,0x53u,0x41u,0x4bu,0x08u,0x09u,0x02u,0x49u,0x53u,0x49u,0x02u,0x03u,
        0x01u,0x54u,0x4fu,0x4du,0x00u,0x01u,0x09u,0x54u,0x41u,0x4bu,0x00u,0x01u,
        0x07u,0x48u,0x41u,0x47u,0x02u,0x03u,0x06u,0x48u,0x41u,0x4eu,0x06u,0x07u,
        0x05u,0x53u,0x49u,0x4eu,0x08u,0x09u,0x04u,0x54u,0x41u,0x54u,0x02u,0x03u,
        0x04u,0x4bu,0x49u,0x4eu,0x00u,0x01u,0x03u,0x4eu,0x41u,0x52u,0x02u,0x03u,
        0x03u,0x4du,0x41u,0x45u,0x06u,0x07u,0x02u,0x53u,0x41u,0x4bu,0x08u,0x09u,
        0x02u,0x49u,0x53u,0x49u,0x02u,0x03u,0x01u,0x54u,0x4fu,0x4du,0x00u,0x01u};
    if(!valid(c))return WF_R18_CONTROL_BAD_CONTEXT;
    memcpy(c->ram_1c0000+0xc2,table_6db0,sizeof table_6db0);
    c->a[0]=0x6db0u;c->a[1]=0x6df2u;c->a[2]=0x1c0104u;c->a[3]=0x1c0146u;
    wf_r17_m68k_move_w(c->regs,0u,0x42u);c->sr&=0xffefu;flags(c,0);return 0;
}
