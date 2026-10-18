#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "wf_r17_post7a00.h"

typedef struct { unsigned n; uint16_t last; int block; } aud_t;
static wf_status_t audio(wf_runtime_t *rt,uint16_t raw,void *u){aud_t*a=u;(void)rt;a->n++;a->last=raw;return a->block?WF_BLOCKED:WF_OK;}
static void init(wf_runtime_t*rt,wf_r17_post7_ctx_t*c,aud_t*a){memset(rt,0,sizeof(*rt));memset(c,0,sizeof(*c));memset(a,0,sizeof(*a));wf_r17_m68k_software_reset(&rt->m68k_regs);rt->m68k_regs.d[0]=0xAAAA5555u;rt->m68k_regs.d[1]=0xBBBB6666u;c->audio_2052=audio;c->user=a;}
static void active(wf_actor_raw_t*a,uint16_t hp){wf_set_u8(a,WF_A_ACTIVE,0x80u);wf_set_be16(a,WF_A_STAMINA,hp);}
static void test_early_gates(void){wf_runtime_t r;wf_r17_post7_ctx_t c;aud_t a;init(&r,&c,&a);active(&r.actors[0],1);assert(wf_r17_owner_9052(&r,&c)==WF_OK);assert(a.n==0&&r.m68k_regs.d[0]==0xAAAA5555u&&r.m68k_regs.d[1]==0xBBBB6666u);r.g.credited_route=1;r.g.global_169e=0x2000;assert(wf_r17_owner_9052(&r,&c)==WF_OK);assert(a.n==0);}
static void test_warning_strict_lt24(void){wf_runtime_t r;wf_r17_post7_ctx_t c;aud_t a;init(&r,&c,&a);r.g.credited_route=1;active(&r.actors[0],24);active(&r.actors[1],23);assert(wf_r17_owner_9052(&r,&c)==WF_OK);assert(a.n==1&&a.last==0x310b);assert(r.g.global_0166==0x8000);assert(r.m68k_regs.d[0]==0xAAAA310Bu);assert(r.m68k_regs.d[1]==0xBBBB0002u);}
static void test_warning_latched(void){wf_runtime_t r;wf_r17_post7_ctx_t c;aud_t a;init(&r,&c,&a);r.g.credited_route=1;r.g.global_0166=0x8000;active(&r.actors[0],1);assert(wf_r17_owner_9052(&r,&c)==WF_OK);assert(a.n==0);assert(r.m68k_regs.d[1]==0xBBBB0003u);}
static void test_progression_table(void){static const uint16_t t[10]={0x3104,0x310a,0x310c,0x310f,0x3106,0x3104,0x310a,0x310c,0x310f,0x3106};for(unsigned k=0;k<10;k++){wf_runtime_t r;wf_r17_post7_ctx_t c;aud_t a;init(&r,&c,&a);r.g.credited_route=1;r.g.progression_0162=(uint16_t)(0xAB00u|k);assert(wf_r17_owner_9052(&r,&c)==WF_OK);assert(a.n==1&&a.last==t[k]);assert(r.g.global_0166==0x4000);assert(r.m68k_regs.d[0]==(0xAAAA0000u|t[k]));assert(r.m68k_regs.d[1]==0xBBBBFFFFu);}}
static void test_bit6_latch(void){wf_runtime_t r;wf_r17_post7_ctx_t c;aud_t a;init(&r,&c,&a);r.g.credited_route=1;r.g.global_0166=0x4000;assert(wf_r17_owner_9052(&r,&c)==WF_OK);assert(a.n==0&&r.m68k_regs.d[1]==0xBBBBFFFFu);}
static void test_block_rollback(void){wf_runtime_t r,b;wf_r17_post7_ctx_t c;aud_t a;init(&r,&c,&a);r.g.credited_route=1;r.g.progression_0162=2;a.block=1;b=r;assert(wf_r17_owner_9052(&r,&c)==WF_BLOCKED);assert(r.g.global_0166==b.g.global_0166);assert(memcmp(&r.m68k_regs,&b.m68k_regs,sizeof(r.m68k_regs))==0);assert(c.blocked_dependency_rom==0x2052u);}
static void test_bad_index_failclosed(void){wf_runtime_t r,b;wf_r17_post7_ctx_t c;aud_t a;init(&r,&c,&a);r.g.credited_route=1;r.g.progression_0162=10;b=r;assert(wf_r17_owner_9052(&r,&c)==WF_BLOCKED);assert(memcmp(&r.m68k_regs,&b.m68k_regs,sizeof(r.m68k_regs))==0);assert(r.g.global_0166==b.g.global_0166);assert(c.blocked_dependency_rom==0x0deeu);}
int main(void){test_early_gates();test_warning_strict_lt24();test_warning_latched();test_progression_table();test_bit6_latch();test_block_rollback();test_bad_index_failclosed();puts("PASS $9052 warning/audio ROM contract");return 0;}
