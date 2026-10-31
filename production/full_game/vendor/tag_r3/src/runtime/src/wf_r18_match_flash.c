#include "wf_r18_match_flash.h"
#include <string.h>
static uint16_t rw(const uint8_t *p) { return (uint16_t)(((unsigned)p[0]<<8)|p[1]); }
static uint32_t rl(const uint8_t *p) { return ((uint32_t)rw(p)<<16)|rw(p+2); }
static void ww(uint8_t *p,uint16_t v) { p[0]=(uint8_t)(v>>8);p[1]=(uint8_t)v; }
static void wl(uint8_t *p,uint32_t v) { ww(p,(uint16_t)(v>>16));ww(p+2,(uint16_t)v); }
static int valid(wf_r18_match_flash_context_t *c) {
    return c&&c->machine&&c->machine->ram&&c->machine->regs&&c->machine->regs->valid&&c->fg_0c0000&&c->fg_bytes>=0x4000u&&c->io;
}
static int input(wf_r18_match_flash_t *s,wf_r18_match_flash_context_t *c,unsigned kind,uint32_t at,uint16_t *value) {
    if(c->io(c->io_user,kind,s->pc,at,value,c->machine)!=WF_OK) {
        s->blocked_dependency=s->pc;c->machine->dependency=s->pc;return WF_R18_PREMATCH_BLOCKED;
    }
    return 0;
}
void wf_r18_match_flash_init(wf_r18_match_flash_t *s,uint32_t owner) { if(s){memset(s,0,sizeof *s);s->pc=owner;s->owner=owner;} }
int wf_r18_match_flash_step(wf_r18_match_flash_t *s,wf_r18_match_flash_context_t *c) {
    if(!s||!valid(c))return WF_R18_PREMATCH_BAD_CONTEXT;
    wf_r18_prematch_t *m=c->machine;uint8_t *ram=m->ram;m->dependency=0;
    switch(s->pc) {
    case 0x8f50u:case 0x8fd6u:
        memcpy(s->saved_d,m->regs->d,sizeof s->saved_d);memcpy(s->saved_a,m->a,sizeof s->saved_a);
        wl(ram+0x80,0);s->pc=s->pc==0x8f50u?0x8f5au:0x8fe0u;return 0;
    case 0x8f5au:case 0x8fe0u:
        m->a[0]=0xc0000u;m->regs->d[1]=rl(ram+0x80)&0xff0000u;
        m->regs->d[0]=s->pc==0x8f5au?0xffbeff04u+m->regs->d[1]:0xffc5ff04u-m->regs->d[1];
        do { wl(c->fg_0c0000+m->a[0]-0xc0000u,m->regs->d[0]);m->a[0]+=4u; } while(m->a[0]<0xc3000u);
        s->pc=s->pc==0x8f5au?0x8f7eu:0x9004u;return 0;
    case 0x8f7eu:case 0x9004u:
        ww(ram+0x6e,0);ram[0x6f]|=1u;s->pc=s->pc==0x8f7eu?0x8f8cu:0x9012u;return 0;
    case 0x8f8cu:case 0x9012u:
        if(!(ram[0x6f]&2u)) {
            uint16_t value=2u;if(input(s,c,WF_R18_PREMATCH_SYNC,0x1c006fu,&value))return WF_R18_PREMATCH_BLOCKED;
            if(!(ram[0x6f]&2u))return WF_R18_PREMATCH_WAIT;
        }
        s->pc=s->pc==0x8f8cu?0x8f96u:0x901cu;return 0;
    case 0x8f96u:case 0x901cu: {
        uint16_t count=(uint16_t)(rw(ram+0x82)+1u);ww(ram+0x82,count);
        if(s->pc==0x8f96u)s->pc=count<3u?0x8f7eu:0x8fa8u;
        else s->pc=count<3u?0x9004u:0x902eu;
        return 0;
    }
    case 0x8fa8u:case 0x902eu: {
        ww(ram+0x82,0);uint16_t color=(uint16_t)(rw(ram+0x80)+1u);ww(ram+0x80,color);
        if(s->pc==0x8fa8u)s->pc=color<8u?0x8f5au:0x8fc0u;
        else s->pc=color<8u?0x8fe0u:0x9046u;
        return 0;
    }
    case 0x8fc0u:
        wf_r17_m68k_move_w(m->regs,0u,0x20u);s->hold_d0=m->regs->d[0];s->pc=0x21eau;return 0;
    case 0x21eau:case 0x21f4u: {
        uint16_t value=0;if(input(s,c,WF_R18_PREMATCH_READ16,0x140026u,&value))return WF_R18_PREMATCH_BLOCKED;
        if(s->pc==0x21eau) {
            if(!(value&0x400u))return WF_R18_PREMATCH_WAIT;
            s->pc=0x21f4u;return 0;
        }
        if(value&0x400u)return WF_R18_PREMATCH_WAIT;
        uint16_t counter=(uint16_t)((uint16_t)m->regs->d[0]-1u);wf_r17_m68k_move_w(m->regs,0u,counter);
        s->pc=counter==0xffffu?0x2202u:0x21eau;return 0;
    }
    case 0x2202u:m->regs->d[0]=s->hold_d0;s->pc=0x8fcau;return 0;
    case 0x8fcau:wl(ram+0x80,0);s->pc=0x8fd0u;return 0;
    case 0x8fd0u:
        memcpy(m->regs->d,s->saved_d,sizeof s->saved_d);memcpy(m->a,s->saved_a,sizeof s->saved_a);s->pc=0x8fd4u;return WF_R18_PREMATCH_DONE;
    case 0x9046u:
        memcpy(m->regs->d,s->saved_d,sizeof s->saved_d);memcpy(m->a,s->saved_a,sizeof s->saved_a);s->pc=0x904au;return 0;
    case 0x904au:wl(ram+0x80,0);s->pc=0x9050u;return WF_R18_PREMATCH_DONE;
    case 0x8fd4u:case 0x9050u:return WF_R18_PREMATCH_DONE;
    default:s->blocked_dependency=s->pc;m->dependency=s->pc;return WF_R18_PREMATCH_BLOCKED;
    }
}
