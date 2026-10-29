#include "wf_r18_frontend_mode_plate.h"
#include <string.h>
static int valid(const wf_r18_title_context_t *c) {
    return c&&c->ram_1c0000&&c->ram_bytes>=WF_R18_TITLE_RAM_BYTES&&c->regs&&c->regs->valid&&c->a&&c->effect;
}
static void w16(uint8_t *p,uint16_t v) { p[0]=(uint8_t)(v>>8);p[1]=(uint8_t)v; }
static int call(wf_r18_frontend_mode_plate_t *s,wf_r18_title_context_t *c,uint32_t owner) {
    wf_r18_title_event_t e={WF_R18_TITLE_CALL,owner,0,0};
    if(c->effect(c->user,&e,c->ram_1c0000,c->regs,c->a)) { s->blocked_dependency=owner;return WF_R18_TITLE_BLOCKED; }
    return 0;
}
void wf_r18_frontend_mode_plate_init(wf_r18_frontend_mode_plate_t *s) { if(s){s->pc=0x52beu;s->blocked_dependency=0;} }
int wf_r18_frontend_mode_plate_step(wf_r18_frontend_mode_plate_t *s,wf_r18_title_context_t *c) {
    if(!s||!valid(c))return WF_R18_TITLE_BAD_CONTEXT;
    uint8_t *ram=c->ram_1c0000;
    switch(s->pc) {
    case 0x52beu:w16(ram+0x15fc,1u);s->pc=0x52c6u;return 0;
    case 0x52c6u:
        if(call(s,c,0x2a06u))return WF_R18_TITLE_BLOCKED;
        s->pc=0x52ccu;return 0;
    case 0x52ccu:case 0x52d6u:case 0x52e0u: {
        uint16_t command=s->pc==0x52ccu?0x3100u:s->pc==0x52d6u?0x3120u:0x3103u;
        uint32_t saved_d0=c->regs->d[0];wf_r17_m68k_move_w(c->regs,0u,command);
        if(call(s,c,0x2052u)) { c->regs->d[0]=saved_d0;return WF_R18_TITLE_BLOCKED; }
        s->pc+=10u;return 0;
    }
    case 0x52eau:
        ram[0x6f]&=0xefu;memset(ram+0x80,0,4);ram[0x76]|=0x80u;
        w16(ram+0x1804,0x140u);w16(ram+0x1806,0x200u);w16(ram+0x7e,3u);s->pc=0x5318u;return 0;
    case 0x5318u:
        if(call(s,c,0x26e66u))return WF_R18_TITLE_BLOCKED;
        s->pc=0x531eu;return 0;
    case 0x531eu:
        if(call(s,c,0x1f9eu))return WF_R18_TITLE_BLOCKED;
        s->pc=0x5324u;return 0;
    case 0x5324u:
        w16(ram+0x15f4,3u);w16(ram+0x15f8,3u);s->pc=0x5334u;return 0;
    case 0x5334u:
        if(call(s,c,0x2a06u))return WF_R18_TITLE_BLOCKED;
        s->pc=0x533au;return WF_R18_TITLE_ATTRACT;
    case 0x533au:return WF_R18_TITLE_ATTRACT;
    default:s->blocked_dependency=s->pc;return WF_R18_TITLE_BLOCKED;
    }
}
