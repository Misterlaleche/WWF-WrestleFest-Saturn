#include "wf_r18_frontend_boot.h"
#include <string.h>

static int valid(const wf_r18_frontend_boot_context_t *c) {
    return c&&c->ram_1c0000&&c->ram_bytes>=WF_R18_TITLE_RAM_BYTES&&c->regs&&c->regs->valid&&c->a;
}
static uint16_t r16(const uint8_t *p) { return (uint16_t)(((unsigned)p[0]<<8)|p[1]); }
static void w16(uint8_t *p,uint16_t v) { p[0]=(uint8_t)(v>>8);p[1]=(uint8_t)v; }
static void w32(uint8_t *p,uint32_t v) { w16(p,(uint16_t)(v>>16));w16(p+2,(uint16_t)v); }
static int emit(wf_r18_frontend_boot_t *s,wf_r18_frontend_boot_context_t *c,
    uint32_t kind,uint32_t owner,uint32_t address,uint32_t value) {
    wf_r18_title_event_t event={kind,owner,address,value};
    if(c->effect(c->user,&event,c->ram_1c0000,c->regs,c->a)) {
        s->blocked_dependency=owner;return WF_R18_BOOT_BLOCKED;
    }
    return 0;
}
static int call(wf_r18_frontend_boot_t *s,wf_r18_frontend_boot_context_t *c,uint32_t owner) {
    return emit(s,c,WF_R18_TITLE_CALL,owner,0,0);
}
static int technos_a0ae(wf_r18_frontend_boot_t *s,wf_r18_frontend_boot_context_t *c) {
    uint32_t saved_d[6],saved_a2=c->a[2];
    memcpy(saved_d,c->regs->d+2,sizeof saved_d);
    c->a[2]=0xc1044u;wf_r17_m68k_move_w(c->regs,2u,0x27u);wf_r17_m68k_move_w(c->regs,7u,0x20u);
    if(call(s,c,0x9acau))return WF_R18_BOOT_BLOCKED;
    memcpy(c->regs->d+2,saved_d,sizeof saved_d);c->a[2]=saved_a2;return 0;
}

void wf_r18_frontend_boot_init(wf_r18_frontend_boot_t *state) {
    if(state){state->pc=0x6fcu;state->blocked_dependency=0;state->hold_return=0;}
}
int wf_r18_frontend_boot_step(wf_r18_frontend_boot_t *state,wf_r18_frontend_boot_context_t *context) {
    if(!state||!valid(context)||!context->effect)return WF_R18_BOOT_BAD_CONTEXT;
    uint8_t *ram=context->ram_1c0000;wf_r17_m68k_shadow_t *r=context->regs;
    static const struct { uint32_t pc,owner,next; } calls[]={
        {0x72eu,0x1fc0u,0x732u},{0x732u,0x1f6cu,0x736u},{0x736u,0x1fdeu,0x73au},{0x73au,0x1f9eu,0x73eu},{0x73eu,0x2988u,0x742u},
        {0x760u,0x2a06u,0x764u},{0x764u,0x26772u,0x76au},{0x77au,0x26e66u,0x780u},{0x780u,0x26844u,0x786u},{0x78au,0x264e2u,0x790u},
        {0x790u,0x1fc0u,0x794u},{0x794u,0x1f6cu,0x798u},{0x798u,0x1fdeu,0x79cu},{0x79cu,0x1f9eu,0x7a0u},{0x7a0u,0x2a06u,0x7a6u},
        {0x7a6u,0x26772u,0x7acu},{0x7b6u,0x26844u,0x7bcu},{0x7c0u,0x264e2u,0x7c6u},{0x7c6u,0x1fc0u,0x7cau},{0x7cau,0x1f6cu,0x7ceu},
        {0x7ceu,0x1fdeu,0x7d2u},{0x7d2u,0x1f9eu,0x7d6u},{0x7f6u,0x2a06u,0x7fcu},{0x7fcu,0x26772u,0x802u},{0x818u,0x26e66u,0x81eu},
        {0x81eu,0x26844u,0x824u},{0x82au,0x264e2u,0x830u},{0x93au,0x1fdeu,0x93eu},{0x93eu,0x1f6cu,0x942u},{0x95au,0x2a06u,0x95eu}};
    for(unsigned i=0;i<sizeof calls/sizeof calls[0];i++)if(state->pc==calls[i].pc) {
        if(call(state,context,calls[i].owner))return WF_R18_BOOT_BLOCKED;
        state->pc=calls[i].next;return WF_R18_BOOT_MORE;
    }
    switch(state->pc) {
    case 0x6fcu:
        w16(ram+0x7c,0);w16(ram+0xb2,0);w16(ram+0x76,0);wf_r18_frontend_clear_1f52(context);
        state->pc=0x714u;return WF_R18_BOOT_MORE;
    case 0x714u:
        ram[0x76]|=0x80u;if(emit(state,context,WF_R18_TITLE_SCROLL16,0x71cu,0x14000cu,0x3100u))return WF_R18_BOOT_BLOCKED;
        state->pc=0x724u;return WF_R18_BOOT_MORE;
    case 0x724u:
        state->pc=(ram[0x67]&0x80u)?0x790u:0x72eu;return WF_R18_BOOT_MORE;
    case 0x742u:
        w16(ram+0x166,0);w16(ram+0x7e,3u);w16(ram+0x15f4,3u);w16(ram+0x15f8,3u);
        state->pc=0x760u;return WF_R18_BOOT_MORE;
    case 0x76au:
        w16(ram+0x1804,0x280u);w16(ram+0x1806,0x600u);state->pc=0x77au;return WF_R18_BOOT_MORE;
    case 0x7acu:
        wf_r17_m68k_move_w(r,0u,4u);if(call(state,context,0x2503cu))return WF_R18_BOOT_BLOCKED;
        state->pc=0x7b6u;return WF_R18_BOOT_MORE;
    case 0x7d6u:
        w16(ram+0x7e,3u);w16(ram+0x15f4,3u);w16(ram+0x15f8,3u);w16(ram+0x15fc,3u);
        state->pc=0x7f6u;return WF_R18_BOOT_MORE;
    case 0x802u:
        if(technos_a0ae(state,context))return WF_R18_BOOT_BLOCKED;
        state->pc=0x808u;return WF_R18_BOOT_MORE;
    case 0x808u:
        w16(ram+0x1804,0x3c0u);w16(ram+0x1806,0x600u);state->pc=0x818u;return WF_R18_BOOT_MORE;
    case 0x786u:case 0x7bcu:case 0x824u:
        state->hold_return=state->pc==0x786u?0x78au:state->pc==0x7bcu?0x7c0u:0x82au;
        wf_r17_m68k_move_w(r,0u,0u);state->pc=0x1e6eu;return WF_R18_BOOT_MORE;
    case 0x1e6eu:
        w16(ram+0x6e,0);ram[0x6f]|=1u;state->pc=0x1e7cu;return WF_R18_BOOT_MORE;
    case 0x1e7cu:
        if(!(ram[0x6f]&2u)) {
            if(emit(state,context,WF_R18_TITLE_SYNC,0x1e7cu,0x1c006fu,2u))return WF_R18_BOOT_BLOCKED;
            if(!(ram[0x6f]&2u))return WF_R18_BOOT_WAIT;
        }
        wf_r17_m68k_move_w(r,0u,(uint16_t)((uint16_t)r->d[0]+1u));
        state->pc=(uint16_t)r->d[0]<128u?0x1e6eu:state->hold_return;return WF_R18_BOOT_MORE;
    case 0x830u:
        state->pc=0x91cu;return WF_R18_BOOT_MORE;
    case 0x91cu:
        wf_r17_m68k_move_w(r,0u,0x3102u);if(call(state,context,0x2052u))return WF_R18_BOOT_BLOCKED;
        state->pc=0x924u;return WF_R18_BOOT_MORE;
    case 0x924u:
        w16(ram+0x7c,0);ram[0x76]|=0x80u;if(call(state,context,0x1fc0u))return WF_R18_BOOT_BLOCKED;
        state->pc=0x936u;return WF_R18_BOOT_MORE;
    case 0x936u:
        wf_r18_frontend_clear_1f38(context);state->pc=0x93au;return WF_R18_BOOT_MORE;
    case 0x942u:
        w32(ram+0x80,0);w16(ram+0x15fc,0);w16(ram+0x15f4,0);w16(ram+0x15f8,0);
        state->pc=0x95au;return WF_R18_BOOT_MORE;
    case 0x95eu:case 0x81bau:
        state->pc=0x81bau;return WF_R18_BOOT_TITLE;
    default:
        state->blocked_dependency=state->pc;return WF_R18_BOOT_BLOCKED;
    }
}
int wf_r18_frontend_clear_1f38(wf_r18_frontend_boot_context_t *context) {
    if(!valid(context))return WF_R18_BOOT_BAD_CONTEXT;
    memset(context->ram_1c0000+0x5ac,0,0x1cd4u-0x5acu);return 0;
}
int wf_r18_frontend_clear_1f52(wf_r18_frontend_boot_context_t *context) {
    if(!valid(context))return WF_R18_BOOT_BAD_CONTEXT;
    memset(context->ram_1c0000+0x160,0,0x1cd4u-0x160u);return 0;
}
int wf_r18_frontend_credit_055a(wf_r18_frontend_boot_context_t *context) {
    if(!valid(context))return WF_R18_BOOT_BAD_CONTEXT;
    uint16_t selector=(uint16_t)context->regs->d[0];
    unsigned amount;
    if(selector==0u)amount=1u;
    else if((uint8_t)selector==4u)amount=2u;
    else return WF_R18_BOOT_BLOCKED;
    uint8_t *credits=context->ram_1c0000+0x4f;
    if(*credits<amount)return 0;
    *credits=(uint8_t)(*credits-amount);return 1;
}
int wf_r18_frontend_start_0978(wf_r18_frontend_boot_context_t *context,const uint16_t ports[4],unsigned *accepted_mask) {
    if(!valid(context)||!ports)return WF_R18_BOOT_BAD_CONTEXT;
    uint8_t *ram=context->ram_1c0000;wf_r17_m68k_shadow_t *r=context->regs;
    uint32_t *a=context->a;
    if(accepted_mask)*accepted_mask=0;
    wf_r17_m68k_move_w(r,0u,(uint16_t)(r16(ram+0x66)&0x800u));
    if((uint16_t)r->d[0]!=0x800u) {
        wf_r17_m68k_move_w(r,1u,3u);a[0]=0x140020u;a[1]=0x1c05b0u;
        for(unsigned slot=0;slot<4u;slot++) {
            if((ports[slot]&0x80u)==0u) {
                wf_r17_m68k_moveq(r,0u,0);
                if(wf_r18_frontend_credit_055a(context)==1) {
                    w16(ram+0x76,0u);ram[0x7c]|=0x80u;wf_r18_frontend_clear_1f38(context);
                    w16(ram+a[1]-0x1c0000u,0x8000u);w32(ram+a[1]-0x1c0000u+0x8au,a[0]);
                    if(accepted_mask){*accepted_mask=1u<<slot;}
                    return 1;
                }
            }
            a[0]+=2u;a[1]+=0x10cu;wf_r17_m68k_move_w(r,1u,(uint16_t)((uint16_t)r->d[1]-1u));
        }
        return 0;
    }
    wf_r17_m68k_move_w(r,0u,(uint16_t)(ports[0]&0x80u));
    if((uint16_t)r->d[0]==0u) {
        wf_r17_m68k_moveq(r,0u,0);
        if(wf_r18_frontend_credit_055a(context)==1) {
            wf_r18_frontend_clear_1f38(context);w16(ram+0x5b0,0x8000u);w32(ram+0x63a,0x140020u);
            ram[0x7c]|=0x80u;
            if(accepted_mask){*accepted_mask=1u;}
            return 1;
        }
    }
    wf_r17_m68k_move_w(r,0u,(uint16_t)(ports[1]&0x80u));
    if((uint16_t)r->d[0]==0u) {
        wf_r17_m68k_moveq(r,0u,4);
        if(wf_r18_frontend_credit_055a(context)==1) {
            wf_r18_frontend_clear_1f38(context);w16(ram+0x5b0,0x8000u);w32(ram+0x63a,0x140020u);
            w16(ram+0x6bc,0x8000u);w32(ram+0x746,0x140022u);ram[0x7c]|=0x80u;
            if(accepted_mask){*accepted_mask=3u;}
            return 1;
        }
    }
    return 0;
}
