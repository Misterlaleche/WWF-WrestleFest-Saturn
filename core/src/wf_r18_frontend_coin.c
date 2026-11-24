#include "wf_r18_frontend_coin.h"
#include <string.h>
static inline __attribute__((always_inline)) int valid(const wf_r18_frontend_control_context_t *c) {
    return c&&c->ram_1c0000&&c->ram_bytes>=WF_R18_TITLE_RAM_BYTES&&c->regs&&c->regs->valid&&c->a&&c->effect;
}
/* Coin owners use fixed D0/D1/D2 indexes. Preserve the public shadow MOVE
 * contract, including upper bits and the null/register guards, while exposing
 * the constant index to SH-2 codegen instead of calling a generic helper. */
static inline __attribute__((always_inline)) void coin_move_b(wf_r17_m68k_shadow_t *r,unsigned reg,uint8_t value) {
    if(r&&reg<8u)r->d[reg]=(r->d[reg]&0xffffff00u)|(uint32_t)value;
}
static inline __attribute__((always_inline)) void coin_move_w(wf_r17_m68k_shadow_t *r,unsigned reg,uint16_t value) {
    if(r&&reg<8u)r->d[reg]=(r->d[reg]&0xffff0000u)|(uint32_t)value;
}
/* These are aligned register words, with exactly three original D registers
 * saved at the IRQ seam. Keep the complete snapshot before writing it back. */
static inline __attribute__((always_inline)) void copy_d012(uint32_t *to,const uint32_t *from) {
    uint32_t d0=from[0],d1=from[1],d2=from[2];
    to[0]=d0;to[1]=d1;to[2]=d2;
}
static uint16_t r16(const uint8_t *p) { return (uint16_t)(((unsigned)p[0]<<8)|p[1]); }
static void w16(uint8_t *p,uint16_t v) { p[0]=(uint8_t)(v>>8);p[1]=(uint8_t)v; }
static void flags(wf_r18_frontend_control_context_t *c,unsigned value,unsigned bits) {
    c->sr=(uint16_t)((c->sr&0xfff0u)|(value==0u?4u:0u)|(value&(1u<<(bits-1u))?8u:0u));
}
/* Every call supplies a constant width, operation and X policy. Keep those
 * constants at the call site: SH-2 otherwise calls three generic libgcc
 * shifts for each CCR update even for a fixed byte/word comparison. */
static inline __attribute__((always_inline)) unsigned arithmetic(wf_r18_frontend_control_context_t *c,unsigned a,unsigned b,unsigned bits,unsigned subtract,unsigned change_x) {
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
    copy_d012(b->d,c->regs->d);b->sr=c->sr;memcpy(b->ram,c->ram_1c0000+0x4e,sizeof b->ram);
}
static int restore(wf_r18_frontend_control_context_t *c,const rollback_t *b) {
    copy_d012(c->regs->d,b->d);c->sr=b->sr;memcpy(c->ram_1c0000+0x4e,b->ram,sizeof b->ram);return WF_R18_CONTROL_BLOCKED;
}
void wf_r18_frontend_coin_enter(wf_r18_frontend_coin_t *s,uint16_t sr,uint32_t pc) {
    /* Preserve every public state byte while exposing the fixed-size clear
     * to SH-2 even under the target's general -fno-builtin setting. */
    if(s){__builtin_memset(s,0,sizeof *s);s->pc=0x434u;s->return_sr=sr;s->return_pc=pc;s->frame_valid=1;}
}
int wf_r18_frontend_coin_step(wf_r18_frontend_coin_t *s,wf_r18_frontend_control_context_t *c) {
    if(!s||!valid(c)||(s->pc==0x434u&&(!(c->sr&0x2000u)||!s->frame_valid)))return WF_R18_CONTROL_BAD_CONTEXT;
    switch(s->pc) {
    case 0x434u:
        copy_d012(s->saved_d,c->regs->d);c->ssp-=12u;s->registers_saved=1;s->pc=0x438u;return 0;
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
        coin_move_w(c->regs,0u,r16(c->ram_1c0000+(port?0x78u:0x54u)));flags(c,(uint16_t)c->regs->d[0],16u);
        coin_move_b(c->regs,1u,(uint8_t)port);flags(c,port,8u);
        coin_move_b(c->regs,2u,c->ram_1c0000[0x10u+port]);flags(c,(uint8_t)c->regs->d[2],8u);
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
        copy_d012(c->regs->d,s->saved_d);c->ssp+=12u;s->registers_saved=0;s->pc=0x48au;return 0;
    case 0x48au:
        if(!s->frame_valid)return WF_R18_CONTROL_BAD_CONTEXT;
        c->sr=(uint16_t)(s->return_sr&0xa71fu);c->ssp+=6u;s->pc=s->return_pc;s->frame_valid=0;return WF_R18_CONTROL_RTE;
    default:s->blocked_dependency=s->pc;return WF_R18_CONTROL_BLOCKED;
    }
}
int wf_r18_frontend_coin_run(wf_r18_frontend_coin_t *s,wf_r18_frontend_control_context_t *c) {
    if(!s||!valid(c)||(s->pc==0x434u&&(!(c->sr&0x2000u)||!s->frame_valid)))return WF_R18_CONTROL_BAD_CONTEXT;
    /* IRQ2 always drains synchronously in the frontend. Dispatch a resume
     * PC once, retaining the public PCs before each observable operation,
     * instead of repeatedly returning, validating and switching per MOVE.
     * The step API remains available for callers that need those yields.
     * Within each move-only span, NZVC is produced by its final byte MOVE;
     * earlier flag writes are dead and none of these moves changes X. */
    switch(s->pc) {
    case 0x434u:goto save_registers;
    case 0x438u:goto acknowledge_start;
    case 0x43eu:goto service;
    case 0x440u:goto port0_load;
    case 0x450u:goto port0_pulse;
    case 0x454u:goto port0_store;
    case 0x460u:goto port1_load;
    case 0x470u:goto port1_pulse;
    case 0x474u:goto port1_store;
    case 0x480u:goto acknowledge_end;
    case 0x486u:goto restore_registers;
    case 0x48au:goto exception_return;
    default:s->blocked_dependency=s->pc;return WF_R18_CONTROL_BLOCKED;
    }
save_registers:
    copy_d012(s->saved_d,c->regs->d);
    c->ssp-=12u;s->registers_saved=1;s->pc=0x438u;
acknowledge_start: {
    uint32_t value=0;
    if(io(c,WF_R18_CONTROL_WRITE16,0x438u,0x140016u,&value)) {
        s->blocked_dependency=0x438u;return WF_R18_CONTROL_BLOCKED;
    }
    flags(c,0,16u);s->pc=0x43eu;
}
service:
    if(wf_r18_frontend_coin_service_04bc(c)) {
        s->blocked_dependency=0x4bcu;return WF_R18_CONTROL_BLOCKED;
    }
    s->pc=0x440u;
port0_load:
    coin_move_w(c->regs,0u,r16(c->ram_1c0000+0x54u));
    coin_move_b(c->regs,1u,0);
    coin_move_b(c->regs,2u,c->ram_1c0000[0x10u]);flags(c,(uint8_t)c->regs->d[2],8u);
    s->pc=0x450u;
port0_pulse:
    if(wf_r18_frontend_coin_pulse_048c(c)) {
        s->blocked_dependency=0x48cu;return WF_R18_CONTROL_BLOCKED;
    }
    s->pc=0x454u;
port0_store:
    w16(c->ram_1c0000+0x54u,(uint16_t)c->regs->d[0]);
    c->ram_1c0000[0x10u]=(uint8_t)c->regs->d[2];flags(c,(uint8_t)c->regs->d[2],8u);
    s->pc=0x460u;
port1_load:
    coin_move_w(c->regs,0u,r16(c->ram_1c0000+0x78u));
    coin_move_b(c->regs,1u,1);
    coin_move_b(c->regs,2u,c->ram_1c0000[0x11u]);flags(c,(uint8_t)c->regs->d[2],8u);
    s->pc=0x470u;
port1_pulse:
    if(wf_r18_frontend_coin_pulse_048c(c)) {
        s->blocked_dependency=0x48cu;return WF_R18_CONTROL_BLOCKED;
    }
    s->pc=0x474u;
port1_store:
    w16(c->ram_1c0000+0x78u,(uint16_t)c->regs->d[0]);
    c->ram_1c0000[0x11u]=(uint8_t)c->regs->d[2];flags(c,(uint8_t)c->regs->d[2],8u);
    s->pc=0x480u;
acknowledge_end: {
    uint32_t value=0;
    if(io(c,WF_R18_CONTROL_WRITE16,0x480u,0x140002u,&value)) {
        s->blocked_dependency=0x480u;return WF_R18_CONTROL_BLOCKED;
    }
    flags(c,0,16u);s->pc=0x486u;
}
restore_registers:
    if(!s->registers_saved)return WF_R18_CONTROL_BAD_CONTEXT;
    copy_d012(c->regs->d,s->saved_d);
    c->ssp+=12u;s->registers_saved=0;s->pc=0x48au;
exception_return:
    if(!s->frame_valid)return WF_R18_CONTROL_BAD_CONTEXT;
    c->sr=(uint16_t)(s->return_sr&0xa71fu);c->ssp+=6u;
    s->pc=s->return_pc;s->frame_valid=0;return WF_R18_CONTROL_RTE;
}
/* Each leaf materializes all CCR bits before an event or its public return.
 * Between those boundaries, dead NZVC calculations may be omitted; X is
 * retained until an original ADD/SUB overwrites it. No input is cached. */
static __attribute__((noinline)) int pulse_credit(wf_r18_frontend_control_context_t *c,uint16_t post_read_sr) {
    /* Only a qualified release needs either rollback snapshot. Keep that
     * transaction out of the ordinary poll's SH-2 register/stack frame. */
    rollback_t before;save(c,&before);before.sr=post_read_sr;
    if(wf_r18_frontend_coin_credit_04f2(c))return restore(c,&before);
    coin_move_b(c->regs,2u,0);
    coin_move_w(c->regs,0u,0);flags(c,0,16u);return 0;
}
int wf_r18_frontend_coin_pulse_048c(wf_r18_frontend_control_context_t *c) {
    if(!valid(c))return WF_R18_CONTROL_BAD_CONTEXT;
    uint32_t value=0;
    if(io(c,WF_R18_CONTROL_READ16,0x48cu,0x140020u,&value))return WF_R18_CONTROL_BLOCKED;
    uint16_t post_read_sr=c->sr;
    unsigned inactive=value&(0x100u<<(c->regs->d[1]&7u));
    if(!inactive) {
        unsigned next=arithmetic(c,(uint16_t)c->regs->d[0],1u,16u,0,1);
        coin_move_w(c->regs,0u,(uint16_t)next);return 0;
    }
    unsigned duration=(uint16_t)c->regs->d[0],release=(uint8_t)c->regs->d[2];
    if(duration==0u) {
        coin_move_w(c->regs,0u,0);
        if(release<10u)coin_move_b(c->regs,2u,(uint8_t)arithmetic(c,release,1u,8u,0,1));
        /* The live post-READ value10 makes CMP.B #10 exactly Z, with X
         * preserved. Keep the complete arithmetic for every other value. */
        else if(release==10u)flags(c,0,8u);
        else arithmetic(c,release,10u,8u,1,0);
        return 0;
    }
    if(release>=4u && duration<0x360u) {
        /* Credit's pre-write ADD.W replaces all CCR bits. On rejection,
         * the outer transaction restores the exact post-read SR image. */
        return pulse_credit(c,post_read_sr);
    }
    coin_move_w(c->regs,0u,0);flags(c,0,16u);return 0;
}
int wf_r18_frontend_coin_service_04bc(wf_r18_frontend_control_context_t *c) {
    if(!valid(c))return WF_R18_CONTROL_BAD_CONTEXT;
    uint32_t value=0;
    if(io(c,WF_R18_CONTROL_READ16,0x4bcu,0x140020u,&value))return WF_R18_CONTROL_BLOCKED;
    uint8_t *ram=c->ram_1c0000;
    if(value&0x400u) {
        if(ram[0x56]>30u)ram[0x4f]=(uint8_t)arithmetic(c,ram[0x4f],1u,8u,0,1);
        ram[0x56]=0;flags(c,0,8u);
    } else if(ram[0x56]&0x80u)flags(c,ram[0x56],8u);
    else ram[0x56]=(uint8_t)arithmetic(c,ram[0x56],1u,8u,0,1);
    return 0;
}
int wf_r18_frontend_coin_credit_04f2(wf_r18_frontend_control_context_t *c) {
    if(!valid(c))return WF_R18_CONTROL_BAD_CONTEXT;
    uint8_t *ram=c->ram_1c0000;rollback_t before;save(c,&before);
    coin_move_w(c->regs,1u,0);
    ram[0x4e]=(uint8_t)(ram[0x4e]+1u);
    w16(ram+0x52,(uint16_t)arithmetic(c,r16(ram+0x52),1u,16u,0,1));
    uint32_t value=0x312au;
    if(io(c,WF_R18_CONTROL_WRITE16,0x500u,0x14000cu,&value))return restore(c,&before);
    coin_move_b(c->regs,1u,ram[0x4b]);
    if((uint8_t)c->regs->d[1]>ram[0x4e]) {
        arithmetic(c,(uint8_t)c->regs->d[1],ram[0x4e],8u,1,0);return 0;
    }
    coin_move_b(c->regs,1u,ram[0x4d]);
    coin_move_w(c->regs,2u,0);
    coin_move_b(c->regs,2u,ram[0x4f]);
    unsigned wide=(uint16_t)c->regs->d[2]+(unsigned)(uint16_t)c->regs->d[1];
    unsigned sum=(uint16_t)wide;
    coin_move_w(c->regs,2u,(uint16_t)sum);
    if(sum>=99u) {
        ram[0x4f]=99u;ram[0x4e]=0;
        /* ADD.W's X survives CMP and both final MOVE/CLR operations. */
        c->sr=(uint16_t)((c->sr&0xffe0u)|(wide>0xffffu?16u:0u)|4u);return 0;
    }
    ram[0x4f]=(uint8_t)(ram[0x4f]+(uint8_t)c->regs->d[1]);
    w16(ram+0x50,(uint16_t)(r16(ram+0x50)+(uint16_t)c->regs->d[1]));
    coin_move_b(c->regs,1u,ram[0x4b]);
    ram[0x4e]=(uint8_t)arithmetic(c,ram[0x4e],(uint8_t)c->regs->d[1],8u,1,1);return 0;
}
