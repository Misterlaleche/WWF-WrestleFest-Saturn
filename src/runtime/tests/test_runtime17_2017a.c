#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "wf_gameplay_bindings.h"
static unsigned clears;
static wf_status_t clear(wf_runtime_t*r,void*u){(void)u;clears++;for(unsigned i=0;i<8;i++)r->m68k_regs.d[i]=0xdeadbeef;return WF_OK;}
static wf_status_t clear_fail(wf_runtime_t*r,void*u){clear(r,u);return WF_BLOCKED;}
static void init(wf_runtime_t*r,wf_gameplay_binding_context_t*c,wf_phase_binding_t*b){size_t n=wf_gameplay_bindings_build(c,b,40);wf_runtime_init(r,b,n);r->g.credited_route=1;r->g.mode_0161=1;c->referee_clear_206fe=clear;for(unsigned i=0;i<8;i++)r->m68k_regs.d[i]=0x12345600+i;}
static void run(wf_runtime_t*r){assert(wf_dispatch_phase(r,0x2017a)==WF_OK);}
int main(void){wf_runtime_t r;wf_gameplay_binding_context_t c;wf_phase_binding_t b[40];uint32_t d[8];
 init(&r,&c,b);r.g.credited_route=0;memcpy(d,r.m68k_regs.d,sizeof(d));run(&r);assert(!memcmp(d,r.m68k_regs.d,sizeof(d)));
 init(&r,&c,b);r.g.global_16c4=1;r.g.global_0156[2]=4;run(&r);assert(r.m68k_regs.d[0]==4&&r.m68k_regs.d[3]==2);
 init(&r,&c,b);r.g.global_16c4=1;run(&r);assert(r.g.global_16c4==3&&r.m68k_regs.d[0]==0&&r.m68k_regs.d[3]==0xffff);
 init(&r,&c,b);r.g.royal_slots_1586[1]=7;run(&r);assert(r.m68k_regs.d[1]==1&&r.m68k_regs.d[3]==2&&r.m68k_regs.d[2]==0x12345602);
 init(&r,&c,b);run(&r);assert(r.g.global_16c4==1&&r.m68k_regs.d[0]==9&&r.m68k_regs.d[1]==0xffff&&r.m68k_regs.d[2]==0xffff&&r.m68k_regs.d[5]==0x12345605&&r.m68k_regs.d[7]==0x12345607);
 init(&r,&c,b);wf_set_u8(&r.actors[2],0,0x80);clears=0;run(&r);assert(clears==1&&r.g.global_16c4==4&&wf_be16(&r.actors[2],0x20)==1&&wf_be16(&r.actors[2],0xae)==12);assert(wf_be16(&r.referee,0x20)==0x8009&&r.m68k_regs.d[0]==3&&r.m68k_regs.d[3]==1&&r.m68k_regs.d[5]==wf_actor_rom_base(2)&&r.m68k_regs.d[7]==0x12345607);assert(wf_u8(&r.actors[4],0)==0x80&&wf_be16(&r.actors[4],0x60)==0x8e);
 init(&r,&c,b);for(unsigned i=0;i<9;i++){wf_set_u8(&r.actors[i],0,0x80);wf_set_u8(&r.actors[i],0x32,0x10);}run(&r);assert(r.g.global_0156[0]==0x3001&&r.g.mode_0160==0x30&&r.g.mode_0161==1&&r.g.progression_0162==0x3001&&r.g.global_0166==0x3001&&r.m68k_regs.d[6]==16);
 init(&r,&c,b);wf_set_u8(&r.actors[0],0,0x80);wf_set_u8(&r.actors[1],0,0x80);clears=0;run(&r);assert(clears==0&&r.m68k_regs.d[3]==2&&r.m68k_regs.d[5]==wf_actor_rom_base(1));
 init(&r,&c,b);wf_set_u8(&r.actors[0],0,0x80);wf_set_u8(&r.actors[1],0,0x80);wf_set_u8(&r.actors[1],0x56,0x80);clears=0;run(&r);assert(clears==0&&r.m68k_regs.d[3]==1&&r.m68k_regs.d[4]==1);
 init(&r,&c,b);wf_set_u8(&r.actors[0],0,0x80);for(unsigned i=4;i<8;i++){wf_set_u8(&r.actors[i],0,0x80);wf_set_u8(&r.actors[i],0x32,0x10);wf_set_u8(&r.actors[i],0x56,0x80);}run(&r);assert(r.m68k_regs.d[0]==0xffff&&r.g.global_0092==wf_actor_rom_base(0));
 init(&r,&c,b);for(unsigned i=4;i<9;i++){wf_set_u8(&r.actors[i],0,0x80);wf_set_u8(&r.actors[i],0x32,0x10);}run(&r);assert(r.g.global_015e==0x3001&&r.g.global_0164==0x3001);
 init(&r,&c,b);wf_set_u8(&r.actors[0],0,0x80);c.referee_clear_206fe=0;assert(wf_dispatch_phase(&r,0x2017a)==WF_BLOCKED&&c.last_blocked_dependency==0x206fe&&r.g.global_16c4==4&&r.g.global_0092==0);
 init(&r,&c,b);r.g.mode_0161=0;memcpy(d,r.m68k_regs.d,sizeof(d));run(&r);assert(!memcmp(d,r.m68k_regs.d,sizeof(d)));
 init(&r,&c,b);r.g.global_16c4=0xa501;run(&r);assert(r.g.global_16c4==0xa503);
 init(&r,&c,b);wf_set_u8(&r.actors[0],0,0x80);r.g.global_16c4=0xa500;r.g.global_169e=0x1234;wf_set_be16(&r.referee,0x20,0x1234);c.referee_clear_206fe=clear_fail;
 assert(wf_dispatch_phase(&r,0x2017a)==WF_BLOCKED);assert(r.g.global_16c4==0xa504&&r.g.global_169e==0x1234&&wf_be16(&r.referee,0x20)==0x1234);
 assert(r.m68k_regs.d[0]==9&&r.m68k_regs.d[1]==0xffff&&r.m68k_regs.d[2]==0xffff&&r.m68k_regs.d[3]==1&&r.m68k_regs.d[4]==0&&r.m68k_regs.d[5]==wf_actor_rom_base(0)&&r.m68k_regs.d[6]==0&&r.m68k_regs.d[7]==0x12345607);
 c.referee_clear_206fe=clear;run(&r);assert(r.g.global_16c4==0xa504&&r.g.global_169e==0&&wf_be16(&r.referee,0x20)==0x8009&&r.g.global_0092==wf_actor_rom_base(0));
 puts("PASS $2017A gates, scans, registers, activation, alias writes");return 0;}
