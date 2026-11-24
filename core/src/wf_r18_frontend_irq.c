#include "wf_r18_frontend_irq.h"
#include <string.h>
static int valid(const wf_r18_frontend_control_context_t *c) {
    return c&&c->ram_1c0000&&c->ram_bytes>=WF_R18_TITLE_RAM_BYTES&&
        c->regs&&c->regs->valid&&c->a&&c->effect;
}
static int emit(wf_r18_frontend_irq_t *s,wf_r18_frontend_control_context_t *c,
    uint32_t kind,uint32_t owner,uint32_t address,uint32_t value) {
    wf_r18_title_event_t e={kind,owner,address,value};
    if(c->effect(c->user,&e,c)) { s->blocked_dependency=owner;return WF_R18_CONTROL_BLOCKED; }
    return 0;
}
void wf_r18_frontend_irq_init(wf_r18_frontend_irq_t *s) { if(s){memset(s,0,sizeof *s);s->pc=0x8aeu;} }
void wf_r18_frontend_irq_enter(wf_r18_frontend_irq_t *s,uint16_t sr,uint32_t pc) {
    if(s){memset(s,0,sizeof *s);s->pc=0x834u;s->return_sr=sr;s->return_pc=pc;s->frame_valid=1;}
}
static void bit_z(wf_r18_frontend_control_context_t *c,unsigned was_set) {
    c->sr=(uint16_t)((c->sr&0xfffbu)|(was_set?0u:4u));
}
int wf_r18_frontend_irq_step(wf_r18_frontend_irq_t *s,wf_r18_frontend_control_context_t *c) {
    if(!s||!valid(c)||((s->pc==0x8aeu||s->pc==0x834u)&&!(c->sr&0x2000u)))return WF_R18_CONTROL_BAD_CONTEXT;
    switch(s->pc) {
    case 0x834u: {
        if(!s->frame_valid)return WF_R18_CONTROL_BAD_CONTEXT;
        wf_r18_title_event_t e={WF_R18_CONTROL_READ16,0x834u,0x140020u,0};
        if(c->effect(c->user,&e,c)) { s->blocked_dependency=0x834u;return WF_R18_CONTROL_BLOCKED; }
        bit_z(c,e.value&0x200u);
        if(!(e.value&0x200u))return WF_R18_CONTROL_WAIT;
        s->pc=0x83eu;return 0;
    }
    case 0x83eu:
        memcpy(s->saved_d,c->regs->d,sizeof s->saved_d);memcpy(s->saved_a,c->a,sizeof s->saved_a);
        c->ssp-=60u;s->registers_saved=1;bit_z(c,c->ram_1c0000[0x76]&0x80u);
        s->pc=c->ram_1c0000[0x76]&0x80u?0x84eu:0x90cu;return 0;
    case 0x84eu: {
        unsigned old=c->ram_1c0000[0x6f]&0x80u;bit_z(c,old);c->ram_1c0000[0x6f]|=0x80u;
        s->pc=old?0x90cu:0x85au;return 0;
    }
    case 0x85au:
        bit_z(c,c->ram_1c0000[0x6f]&1u);s->pc=c->ram_1c0000[0x6f]&1u?0x866u:0x904u;return 0;
    case 0x866u:
        if(emit(s,c,WF_R18_CONTROL_WRITE16,0x866u,0x140008u,0))return WF_R18_CONTROL_BLOCKED;
        c->sr=(uint16_t)((c->sr&0xfff0u)|4u);s->pc=0x86eu;return 0;
    case 0x86eu:case 0x874u:case 0x87au:case 0x880u:case 0x886u:case 0x88cu:case 0x892u:case 0x898u: {
        static const uint32_t owners[8]={0xb4b8u,0xb53au,0x27012u,0x27066u,0x26e1au,0x2946au,0x26dacu,0x264aau};
        uint32_t owner=owners[(s->pc-0x86eu)/6u];
        if(emit(s,c,WF_R18_CONTROL_CALL,owner,0,0))return WF_R18_CONTROL_BLOCKED;
        s->pc+=6u;return 0;
    }
    case 0x89eu:
        bit_z(c,c->ram_1c0000[0x7c]&0x80u);s->pc=c->ram_1c0000[0x7c]&0x80u?0x8f8u:0x8a8u;return 0;
    case 0x8a8u:
        if(emit(s,c,WF_R18_CONTROL_CALL,0x978u,0,0))return WF_R18_CONTROL_BLOCKED;
        s->pc=0x8acu;return 0;
    case 0x8acu:s->pc=c->sr&1u?0x8aeu:0x8f8u;return 0;
    case 0x8aeu:
        c->ssp=0x1c3bd4u;c->a[0]=0x1c3fd4u;c->usp=c->a[0];
        c->ram_1c0000[0x76]=0;c->ram_1c0000[0x77]=0;
        c->sr=(uint16_t)((c->sr&0xfff0u)|4u);s->pc=0x8c4u;return 0;
    case 0x8c4u:
        if(emit(s,c,WF_R18_CONTROL_WRITE16,0x8c4u,0x140000u,0))return WF_R18_CONTROL_BLOCKED;
        c->sr=(uint16_t)((c->sr&0xfff0u)|4u);s->pc=0x8cau;return 0;
    case 0x8cau: {
        uint8_t old=c->ram_1c0000[0x6f];
        c->ram_1c0000[0x6f]=(uint8_t)((old&0x7fu)|2u);
        c->sr=(uint16_t)((c->sr&0xfffbu)|((old&2u)?0u:4u));
        uint16_t captured=(uint16_t)(c->sr&0xdcffu);
        wf_r17_m68k_move_w(c->regs,0u,captured);
        c->sr=(uint16_t)(captured&0xa71fu);s->pc=0x8e6u;return 0;
    }
    case 0x8e6u:case 0x8eau:case 0x8eeu: {
        uint32_t owner=s->pc==0x8e6u?0x1f6cu:s->pc==0x8eau?0x1fdeu:0x1f9eu;
        if(emit(s,c,WF_R18_CONTROL_CALL,owner,0,0))return WF_R18_CONTROL_BLOCKED;
        s->pc+=4u;return 0;
    }
    case 0x8f2u:case 0x52beu:s->pc=0x52beu;return WF_R18_CONTROL_HANDOFF;
    case 0x8f8u:
        bit_z(c,c->ram_1c0000[0x6f]&2u);c->ram_1c0000[0x6f]|=2u;s->pc=0x900u;return 0;
    case 0x900u:
        if(emit(s,c,WF_R18_CONTROL_CALL,0x1e92u,0,0))return WF_R18_CONTROL_BLOCKED;
        s->pc=0x904u;return 0;
    case 0x904u:
        bit_z(c,c->ram_1c0000[0x6f]&0x80u);c->ram_1c0000[0x6f]&=0x7fu;s->pc=0x90cu;return 0;
    case 0x90cu:
        if(!s->registers_saved)return WF_R18_CONTROL_BAD_CONTEXT;
        memcpy(c->regs->d,s->saved_d,sizeof s->saved_d);memcpy(c->a,s->saved_a,sizeof s->saved_a);
        c->ssp+=60u;s->registers_saved=0;s->pc=0x910u;return 0;
    case 0x910u:
        if(emit(s,c,WF_R18_CONTROL_WRITE16,0x910u,0x140000u,0))return WF_R18_CONTROL_BLOCKED;
        c->sr=(uint16_t)((c->sr&0xfff0u)|4u);s->pc=0x918u;return 0;
    case 0x918u:
        if(!s->frame_valid)return WF_R18_CONTROL_BAD_CONTEXT;
        c->sr=(uint16_t)(s->return_sr&0xa71fu);c->ssp+=6u;s->pc=s->return_pc;s->frame_valid=0;
        return WF_R18_CONTROL_RTE;
    default:s->blocked_dependency=s->pc;return WF_R18_CONTROL_BLOCKED;
    }
}
