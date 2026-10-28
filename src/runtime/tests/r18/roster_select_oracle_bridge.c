#include <stddef.h>
#include <stdint.h>
#include <string.h>
#if defined(WF_R18_ROSTER_IMPLEMENTED)
#include "wf_r18_roster_select.h"
typedef struct bridge {uint8_t *memory;uint32_t *events;unsigned count;} bridge_t;
static int read_rom(void *user,uint32_t at,void *dst,size_t bytes){
    bridge_t *b=user;if(at>0x80000u||bytes>0x80000u-at)return -1;
    memcpy(dst,b->memory+at,bytes);return 0;
}
static int effect(void *user,uint32_t pc,uint32_t a[7],uint8_t *ram,
                  wf_r17_m68k_shadow_t *regs){
    bridge_t *b=user;uint32_t *event=b->events+1u+b->count++*10u;
    event[0]=pc;event[1]=a[0];memcpy(event+2,regs->d,sizeof regs->d);
    (void)ram;return 0;
}
#if defined(WF_R18_ROSTER_INIT_IMPLEMENTED)
static int palette(void *user,uint32_t pc,uint32_t at,uint16_t value){
    bridge_t *b=user;uint32_t *event=b->events+1u+b->count++*10u;
    event[0]=pc;event[1]=at;event[2]=value;
    memset(event+3,0,7u*sizeof *event);
    if(at>0x1ffffeu)return -1;
    b->memory[at]=(uint8_t)(value>>8);b->memory[at+1]=(uint8_t)value;return 0;
}
#endif
#endif

int roster_select_oracle(uint8_t *memory,uint32_t *d,unsigned owner,
                         unsigned actor_offset,uint32_t *events){
#if defined(WF_R18_ROSTER_IMPLEMENTED)
    wf_r17_m68k_shadow_t regs;wf_r18_roster_select_context_t ctx;
    bridge_t bridge={memory,events,0};int result;
    memset(&ctx,0,sizeof ctx);memcpy(regs.d,d,sizeof regs.d);regs.valid=1;
    ctx.ram_1c0000=memory+0x1c0000;ctx.ram_bytes=0x2000;ctx.regs=&regs;
    ctx.a[0]=0x1c0000u+actor_offset;
    ctx.read_rom=read_rom;ctx.effect=effect;ctx.user=&bridge;
    for(unsigned i=0;i<4;i++)ctx.input_words[i]=0xffffu;
    if(owner==0x658cu)result=wf_r18_roster_scroll_658c(&ctx);
    else if(owner==0x600eu)result=wf_r18_roster_reconcile_600e(&ctx);
    else if(owner==0x5b3eu)result=wf_r18_roster_navigate_5b3e(&ctx,actor_offset);
    else return -98;
    memcpy(d,regs.d,sizeof regs.d);events[0]=bridge.count;events[320]=ctx.blocked_dependency;return result;
#else
    (void)memory;(void)d;(void)owner;(void)actor_offset;events[0]=0;
    return -99; /* Test-only missing-feature probe, never linked into Saturn. */
#endif
}

int roster_select_init_oracle(uint8_t *memory,uint32_t *d,uint32_t *cursor,
                             const uint16_t *ports,uint32_t *events){
#if defined(WF_R18_ROSTER_INIT_IMPLEMENTED)
    wf_r17_m68k_shadow_t regs;wf_r18_roster_select_context_t ctx;
    wf_r18_roster_select_t state;bridge_t bridge={memory,events,0};int result;
    memset(&ctx,0,sizeof ctx);memcpy(regs.d,d,sizeof regs.d);regs.valid=1;
    ctx.ram_1c0000=memory+0x1c0000;ctx.ram_bytes=0x2000;ctx.regs=&regs;
    memcpy(ctx.a,cursor+1,sizeof ctx.a);memcpy(ctx.input_words,ports,sizeof ctx.input_words);
    ctx.read_rom=read_rom;ctx.effect=effect;ctx.write_palette=palette;ctx.user=&bridge;state.pc=cursor[0];
    result=wf_r18_roster_select_step(&state,&ctx);
    cursor[0]=state.pc;memcpy(cursor+1,ctx.a,sizeof ctx.a);
    memcpy(d,regs.d,sizeof regs.d);events[0]=bridge.count;events[320]=ctx.blocked_dependency;
    return result;
#else
    (void)memory;(void)d;(void)cursor;(void)ports;events[0]=0;return -99;
#endif
}
