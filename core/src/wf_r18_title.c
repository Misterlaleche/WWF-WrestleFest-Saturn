#include "wf_r18_title.h"
#include <string.h>
static unsigned portrait_divisor=1u;
void wf_r18_title_portrait_divisor_set(unsigned divisor){portrait_divisor=divisor==2u?2u:1u;}

static uint16_t r16(const uint8_t *p) { return (uint16_t)(((unsigned)p[0]<<8)|p[1]); }
static void w16(uint8_t *p,uint16_t v) { p[0]=(uint8_t)(v>>8);p[1]=(uint8_t)v; }
static void add16(uint8_t *p,unsigned v) { w16(p,(uint16_t)(r16(p)+v)); }
static int event(wf_r18_title_t *s,wf_r18_title_context_t *c,uint32_t kind,
                 uint32_t owner,uint32_t address,uint32_t value) {
    wf_r18_title_event_t e={kind,owner,address,value};
    if(c->effect(c->user,&e,c->ram_1c0000,c->regs,c->a)) {
        s->blocked_dependency=owner;return WF_R18_TITLE_BLOCKED;
    }
    return WF_R18_TITLE_MORE;
}
static int call(wf_r18_title_t *s,wf_r18_title_context_t *c,uint32_t owner) {
    return event(s,c,WF_R18_TITLE_CALL,owner,0,0);
}
static void scroll_add(uint8_t *ram) { add16(ram+0x1cd4,2u);add16(ram+0x1cd6,1u); }
static void request(uint8_t *ram) { w16(ram+0x6e,(uint16_t)(r16(ram+0x6e)&0x10u));ram[0x6f]|=1u; }
static int ack(wf_r18_title_t *s,wf_r18_title_context_t *c) {
    if(!(c->ram_1c0000[0x6f]&2u)) {
        if(event(s,c,WF_R18_TITLE_SYNC,s->pc,0x1c006fu,2u))return WF_R18_TITLE_BLOCKED;
        if(!(c->ram_1c0000[0x6f]&2u))return WF_R18_TITLE_WAIT;
    }
    return WF_R18_TITLE_MORE;
}
static int scroll_commit(wf_r18_title_t *s,wf_r18_title_context_t *c,uint32_t first,uint32_t second) {
    if(event(s,c,WF_R18_TITLE_SCROLL16,first,0x100000u,r16(c->ram_1c0000+0x1cd4)))return WF_R18_TITLE_BLOCKED;
    return event(s,c,WF_R18_TITLE_SCROLL16,second,0x100004u,r16(c->ram_1c0000+0x1cd6));
}
static uint8_t *object(wf_r18_title_t *s,wf_r18_title_context_t *c) {
    if(c->a[0]<0x1c0000u||c->a[0]>0x1c0000u+WF_R18_TITLE_RAM_BYTES-0x2au) {
        s->blocked_dependency=s->pc;return 0;
    }
    return c->ram_1c0000+c->a[0]-0x1c0000u;
}
static int a44e(wf_r18_title_t *s,wf_r18_title_context_t *c) {
    uint32_t saved_d[8],saved_a[7];
    memcpy(saved_d,c->regs->d,sizeof saved_d);memcpy(saved_a,c->a,sizeof saved_a);
    static const uint32_t destinations[3]={0xc0344u,0xc050cu,0xc0a10u};
    static const uint16_t palettes[3]={0x10u,0x60u,0x70u};
    for(unsigned i=0;i<3u;i++) {
        c->a[2]=destinations[i];wf_r17_m68k_move_w(c->regs,2u,(uint16_t)(13u+i));
        wf_r17_m68k_move_w(c->regs,7u,palettes[i]);
        if(call(s,c,0x9acau))return WF_R18_TITLE_BLOCKED;
    }
    memcpy(c->regs->d,saved_d,sizeof saved_d);memcpy(c->a,saved_a,sizeof saved_a);
    c->ram_1c0000[0xf1d]|=0x80u;
    return WF_R18_TITLE_MORE;
}

void wf_r18_title_init(wf_r18_title_t *state) {
    if (state) { state->pc=0x81bau; state->blocked_dependency=0; }
}
int wf_r18_title_step(wf_r18_title_t *state, wf_r18_title_context_t *context) {
    if(!state||!context||!context->ram_1c0000||context->ram_bytes<WF_R18_TITLE_RAM_BYTES||
       !context->regs||!context->regs->valid||!context->a||!context->effect)
        return WF_R18_TITLE_BAD_CONTEXT;
    uint8_t *ram=context->ram_1c0000;
    wf_r17_m68k_shadow_t *r=context->regs;
    uint32_t *a=context->a;
    uint8_t *o;
    int status;
    switch(state->pc) {
    case 0x81bau:
        w16(ram+0x15f4,3u);w16(ram+0x15f8,3u);state->pc=0x81cau;return WF_R18_TITLE_MORE;
    case 0x81cau:
        if(event(state,context,WF_R18_TITLE_CALL,0x2a06u,0,0))return WF_R18_TITLE_BLOCKED;
        state->pc=0x81d0u;return WF_R18_TITLE_MORE;
    case 0x81d0u:
        w16(ram+0x7e,3u);w16(ram+0x1804,0x280u);w16(ram+0x1806,0x400u);
        if(call(state,context,0x26e66u))return WF_R18_TITLE_BLOCKED;
        state->pc=0x81eeu;return WF_R18_TITLE_MORE;
    case 0x81eeu:
        ram[0x6f]|=0x10u;
        if(call(state,context,0x1f9eu))return WF_R18_TITLE_BLOCKED;
        w16(ram+0x72,0xffffu);w16(ram+0x1cd4,0);w16(ram+0x1cd6,0);w16(ram+0x1cd8,0);
        state->pc=0x821au;return WF_R18_TITLE_MORE;
    case 0x821au:
        scroll_add(ram);request(ram);state->pc=0x823au;return WF_R18_TITLE_MORE;
    case 0x823au:
        status=ack(state,context);if(status)return status;
        if(scroll_commit(state,context,0x8244u,0x824eu))return WF_R18_TITLE_BLOCKED;
        add16(ram+0x1cd8,1u);
        if(r16(ram+0x1cd8)<128u)state->pc=0x821au;
        else { a[0]=0x1c1258u;wf_r17_m68k_move_w(r,0u,0u);state->pc=0x8272u; }
        return WF_R18_TITLE_MORE;
    case 0x8272u: {
        static const uint16_t objects[12][3]={
            {0x20u,120u,96u},{0x21u,184u,96u},{0x24u,56u,96u},{0x25u,248u,96u},
            {0x23u,120u,160u},{0x22u,184u,16u},{0x26u,184u,160u},{0x29u,120u,16u},
            {0x28u,248u,160u},{0x2au,56u,16u},{0x27u,56u,160u},{0x2bu,248u,16u}};
        o=object(state,context);if(!o)return WF_R18_TITLE_BLOCKED;
        unsigned index=(uint16_t)r->d[0];
        if(index>=12u){state->blocked_dependency=0x8284u;return WF_R18_TITLE_BLOCKED;}
        w16(o,0x8000u);wf_r17_m68k_move_l(r,1u,index*6u);a[1]=0x86c8u;
        w16(o+2,objects[index][0]);w16(o+0x14,objects[index][1]);w16(o+0x16,objects[index][2]);
        state->pc=0x8296u;return WF_R18_TITLE_MORE;
    }
    case 0x8296u: {
        o=object(state,context);if(!o)return WF_R18_TITLE_BLOCKED;
        w16(o+4,r16(o+0x24));
        uint32_t saved_d0=r->d[0],saved_d1=r->d[1],saved_a0=a[0];
        wf_r17_m68k_move_w(r,1u,11u);a[0]=0x1c1258u;
        for(unsigned iteration=0;iteration<12u;iteration++) {
            o=object(state,context);if(!o)return WF_R18_TITLE_BLOCKED;
            wf_r17_m68k_move_w(r,0u,r16(o+2));if(call(state,context,0x2aeau))return WF_R18_TITLE_BLOCKED;
            wf_r17_m68k_move_w(r,0u,0x2cu);if(call(state,context,0x2aeau))return WF_R18_TITLE_BLOCKED;
            wf_r17_m68k_move_w(r,0u,0x2du);if(call(state,context,0x2aeau))return WF_R18_TITLE_BLOCKED;
            if(call(state,context,0x27b8u))return WF_R18_TITLE_BLOCKED;
            a[0]+=0x2au;wf_r17_m68k_move_w(r,1u,(uint16_t)((uint16_t)r->d[1]-1u));
        }
        r->d[0]=saved_d0;r->d[1]=saved_d1;a[0]=saved_a0;
        scroll_add(ram);if(call(state,context,0x2836u))return WF_R18_TITLE_BLOCKED;
        request(ram);state->pc=0x8302u;return WF_R18_TITLE_MORE;
    }
    case 0x8302u:
        status=ack(state,context);if(status)return status;
        if(scroll_commit(state,context,0x830cu,0x8316u))return WF_R18_TITLE_BLOCKED;
        o=object(state,context);if(!o)return WF_R18_TITLE_BLOCKED;
        add16(o+0x22,portrait_divisor);wf_r17_m68k_move_w(r,7u,r16(o+0x24)==4u?25u:3u);
        if((uint16_t)r->d[7]>=r16(o+0x22)){state->pc=0x8296u;return WF_R18_TITLE_MORE;}
        w16(o+0x22,0u);add16(o+0x24,1u);
        if(r16(o+0x24)<5u){state->pc=0x8296u;return WF_R18_TITLE_MORE;}
        w16(o+0x24,0u);a[0]+=0x2au;wf_r17_m68k_move_w(r,0u,(uint16_t)((uint16_t)r->d[0]+1u));
        if((uint16_t)r->d[0]<12u){state->pc=0x8272u;return WF_R18_TITLE_MORE;}
        w16(ram+0x1cd8,0u);wf_r17_m68k_move_w(r,0u,1u);
        if(call(state,context,0x2503cu))return WF_R18_TITLE_BLOCKED;
        state->pc=0x8378u;return WF_R18_TITLE_MORE;
    case 0x8378u:
        if(a44e(state,context))return WF_R18_TITLE_BLOCKED;
        state->pc=0x837eu;return WF_R18_TITLE_MORE;
    case 0x837eu:
        scroll_add(ram);w16(ram+0x6e,(uint16_t)(r16(ram+0x6e)&0x10u));
        if(call(state,context,0xb312u))return WF_R18_TITLE_BLOCKED;
        ram[0x6f]|=1u;state->pc=0x83a4u;return WF_R18_TITLE_MORE;
    case 0x83a4u:
        status=ack(state,context);if(status)return status;
        if(scroll_commit(state,context,0x83aeu,0x83b8u))return WF_R18_TITLE_BLOCKED;
        add16(ram+0x1cd8,1u);
        if(ram[0x1cd9]&0x40u) {
            wf_r17_m68k_move_w(r,0u,0x8056u);if(call(state,context,0x2503cu))return WF_R18_TITLE_BLOCKED;
            wf_r17_m68k_move_w(r,0u,0x8054u);if(call(state,context,0x2503cu))return WF_R18_TITLE_BLOCKED;
            uint8_t old=ram[0x1278];ram[0x1278]|=0x80u;
            if(!(old&0x80u)){ram[0x1279]++;if(ram[0x1279]==2u)ram[0x1279]=0u;}
        } else {
            ram[0x1278]=0u;
            if(ram[0x4f]==0u) {
                wf_r17_m68k_move_w(r,0u,2u);
                if(r16(ram+0x4e)!=0u) {
                    wf_r17_m68k_move_w(r,0u,0x53u);
                    if(ram[0x4e]!=2u) {
                        wf_r17_m68k_move_w(r,0u,0x54u);wf_r17_m68k_move_w(r,1u,(uint16_t)(r16(ram+0x66)&3u));
                        if((uint16_t)r->d[1]!=3u)wf_r17_m68k_move_w(r,0u,0x53u);
                    }
                }
            } else {
                wf_r17_m68k_move_w(r,0u,3u);
                if(ram[0x66]&8u) {
                    wf_r17_m68k_move_w(r,0u,r16(ram+0x1278));
                    if(ram[0x4f]==1u)wf_r17_m68k_move_w(r,0u,0u);
                    wf_r17_m68k_move_w(r,0u,(uint16_t)(((uint16_t)r->d[0]&0xffu)+0x55u));
                }
            }
            if(call(state,context,0x2503cu))return WF_R18_TITLE_BLOCKED;
        }
        state->pc=0x8482u;return WF_R18_TITLE_MORE;
    case 0x8482u:
        state->pc=r16(ram+0x1cd8)<1480u?0x837eu:0x848eu;return WF_R18_TITLE_MORE;
    case 0x848eu:
        state->pc=0x962u;return WF_R18_TITLE_MORE;
    case 0x962u:
        ram[0x6f]&=0xefu;wf_r17_m68k_move_w(r,0u,0x3101u);
        if(call(state,context,0x2052u))return WF_R18_TITLE_BLOCKED;
        state->pc=0x974u;return WF_R18_TITLE_MORE;
    case 0x974u:
        state->pc=0xac0u;return WF_R18_TITLE_ATTRACT;
    case 0xac0u:
        return WF_R18_TITLE_ATTRACT;
    default:
        state->blocked_dependency=state->pc;return WF_R18_TITLE_BLOCKED;
    }
}
