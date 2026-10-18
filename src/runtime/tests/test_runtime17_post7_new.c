#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "wf_r17_post7a00.h"

typedef struct sink {
    wf_r17_fg0_write_t fg[64]; size_t nfg;
    uint16_t res_load[32]; size_t nload;
    uint16_t res_free[32]; size_t nfree;
    uint16_t raw2503[32]; size_t n2503;
    uint16_t d2,d7; uint32_t dest; size_t n9aca;
    wf_actor_raw_t *rendered[64]; size_t nrender; int block_render;
    size_t n9132; wf_actor_raw_t *target9132; uint16_t paid9132; size_t n6e34;
} sink_t;
static wf_status_t fg(wf_runtime_t *r,const wf_r17_fg0_write_t *w,size_t n,void *u){sink_t*s=u;(void)r;assert(n<=64u);memcpy(s->fg,w,n*sizeof(*w));s->nfg=n;return WF_OK;}
static wf_status_t load(wf_runtime_t*r,uint16_t v,void*u){sink_t*s=u;(void)r;s->res_load[s->nload++]=v;return WF_OK;}
static wf_status_t free_(wf_runtime_t*r,uint16_t v,void*u){sink_t*s=u;(void)r;s->res_free[s->nfree++]=v;return WF_OK;}
static wf_status_t raw2503(wf_runtime_t*r,uint16_t v,void*u){sink_t*s=u;(void)r;s->raw2503[s->n2503++]=v;return WF_OK;}
static wf_status_t raw9(wf_runtime_t*r,uint16_t d2,uint16_t d7,uint32_t dest,void*u){sink_t*s=u;(void)r;s->d2=d2;s->d7=d7;s->dest=dest;s->n9aca++;return WF_OK;}
static wf_status_t tag6e34(wf_runtime_t*r,void*u){sink_t*s=u;(void)r;s->n6e34++;return WF_OK;}
static wf_status_t royal9132(wf_runtime_t*r,wf_actor_raw_t*t,uint16_t paid,void*u){sink_t*s=u;(void)r;s->n9132++;s->target9132=t;s->paid9132=paid;return WF_OK;}
static wf_status_t render2836(wf_runtime_t*r,wf_actor_raw_t *const *a,size_t n,void*u){sink_t*s=u;if(s->block_render)return WF_BLOCKED;assert(n<=64u);for(size_t i=0;i<n;i++)s->rendered[i]=a[i];s->nrender=n;/* demonstrate caller-visible D6/D7: WP26 owns exact $D1FC effects */r->m68k_regs.d[6]=0x66660001u;r->m68k_regs.d[7]=0u;return WF_OK;}
static void init(wf_runtime_t *r,wf_r17_post7_ctx_t *c,sink_t*s,wf_r17_render_ctx_t*rc,wf_r17_render_queue_t*q){
 memset(r,0,sizeof(*r));memset(c,0,sizeof(*c));memset(s,0,sizeof(*s));memset(rc,0,sizeof(*rc));wf_r17_render_queue_clear(q);wf_r17_m68k_software_reset(&r->m68k_regs);rc->queue=q;rc->visibility_2188=wf_r17_visibility_2188_exact;c->render=rc;c->fg0_batch=fg;c->resource_2aea=load;c->resource_2b58=free_;c->raw_2503c=raw2503;c->raw_9aca=raw9;c->render_2836_batch=render2836;c->tag_6e34=tag6e34;c->royal_9132=royal9132;c->user=s;
}
static void test_88a2(void){wf_runtime_t r;wf_r17_post7_ctx_t c;sink_t s;wf_r17_render_ctx_t rc;wf_r17_render_queue_t q;init(&r,&c,&s,&rc,&q);r.m68k_regs.d[0]=0xaaaa1111u;r.m68k_regs.d[2]=0xbbbb2222u;wf_set_u8(&r.actors[0],WF_A_FLAGS33,0x20u);wf_set_u8(&r.actors[0],0xcfu,2u);wf_set_be16(&r.actors[0],0xccu,1u);wf_set_u8(&r.actors[1],0xceu,0x80u);wf_set_be16(&r.actors[1],0xc2u,0x1234u);wf_set_be16(&r.actors[1],0x96u,0x5678u);assert(wf_r17_owner_88a2(&r,&c)==WF_OK);assert(s.nfg==12u);assert(s.fg[0].addr==0x000c0110u&&s.fg[0].value==0xffcc0013u);assert(s.fg[1].addr==0x000c0210u&&s.fg[1].value==0xffd20013u);assert(wf_u8(&r.actors[0],0xceu)==0x80u&&wf_u8(&r.actors[0],0xcfu)==3u&&wf_be16(&r.actors[0],0xccu)==0u);assert(wf_be16(&r.actors[1],0xc2u)==0u&&wf_be16(&r.actors[1],0x96u)==0u);assert((uint16_t)r.m68k_regs.d[0]==4u);assert((uint16_t)r.m68k_regs.d[2]==0xffffu);puts("PASS $88A2");}
static void test_8710(void){wf_runtime_t r;wf_r17_post7_ctx_t c;sink_t s;wf_r17_render_ctx_t rc;wf_r17_render_queue_t q;init(&r,&c,&s,&rc,&q);r.g.credited_route=1u;wf_set_u8(&r.actors[0],WF_A_ACTIVE,0x80u);wf_set_u8(&r.actors[0],WF_A_PINFLAGS,0x08u);wf_set_be16(&r.actors[0],WF_A_X,0x0200u);wf_set_be16(&r.actors[0],WF_A_Y,0x0100u);wf_set_be16(&r.actors[0],WF_A_Z,0x0020u);wf_set_u8(&r.actors[0],WF_A_FACING,0x80u);assert(wf_r17_owner_8710(&r,&c)==WF_OK);assert(s.nload==1u&&s.res_load[0]==0x35u);assert((wf_u8(&r.compact_14ce[0],0)&0x80u)!=0);assert(wf_be16(&r.compact_14ce[0],WF_A_X)==0x0208u);assert(wf_be16(&r.compact_14ce[0],WF_A_Y)==0x0100u);assert(wf_be16(&r.compact_14ce[0],WF_A_Z)==0x0088u);assert((uint16_t)r.m68k_regs.d[6]==0xffffu);puts("PASS $8710");}
static void test_bb14_fallback_lowest_free(void){wf_runtime_t r;wf_r17_post7_ctx_t c;sink_t s;wf_r17_render_ctx_t rc;wf_r17_render_queue_t q;unsigned i;init(&r,&c,&s,&rc,&q);r.g.global_16a7=1u;r.g.global_1694=0x0125u;r.g.rng_state=381u;r.g.rng_state_valid=1u;for(i=0;i<6u;i++)r.g.royal_occupancy[i]=1u;r.g.royal_occupancy[0]=0u;for(i=6u;i<12u;i++)r.g.royal_occupancy[i]=1u;assert(wf_r17_owner_bb14(&r,&c)==WF_OK);assert(r.g.global_16a7==0u);assert(r.g.royal_occupancy[0]==1u);assert(r.g.royal_slots_1586[4]==0x4000u);assert(wf_be16(&r.actors[4],WF_A_ROLE)==0x8000u);assert(wf_be16(&r.actors[4],WF_A_CHARACTER)==0x0010u);assert(wf_be16(&r.actors[4],WF_A_STAMINA)==0x0087u);puts("PASS $BB14 fallback 0->5");}
static void test_7366(void){wf_runtime_t r;wf_r17_post7_ctx_t c;sink_t s;wf_r17_render_ctx_t rc;wf_r17_render_queue_t q;init(&r,&c,&s,&rc,&q);r.g.mode_0161=1u;r.g.royal_slots_1586[0]=0x2000u;wf_set_u8(&r.actors[0],WF_A_ACTIVE,0x80u);wf_set_u8(&r.actors[0],WF_A_LATCHED,0x80u);wf_set_be16(&r.actors[0],WF_A_ANIM_COUNTER,2u);wf_set_be16(&r.actors[0],WF_A_RENDER_LIST,0u);assert(wf_r17_owner_7366(&r,&c)==WF_OK);assert(wf_be16(&r.actors[0],WF_A_STATE)==0u);assert(wf_be16(&r.actors[0],WF_A_ANIM_COUNTER)==1u);assert(wf_be16(&r.actors[0],WF_A_SCREEN_X)==0x0100u&&wf_be16(&r.actors[0],WF_A_SCREEN_Y)==0x0090u);assert(q.count[0]==1u);puts("PASS $7366");}
static void test_8cf8_early(void){wf_runtime_t r;wf_r17_post7_ctx_t c;sink_t s;wf_r17_render_ctx_t rc;wf_r17_render_queue_t q;init(&r,&c,&s,&rc,&q);r.g.global_0166=0u;r.m68k_regs.d[0]=0xaaaa1111u;r.m68k_regs.d[2]=0xbbbb2222u;r.m68k_regs.d[7]=0xcccc3333u;assert(wf_r17_owner_8cf8(&r,&c)==WF_OK);assert(s.n2503==1u&&s.raw2503[0]==0x29u);assert(s.n9aca==1u&&s.d2==0x15u&&s.d7==0x70u&&s.dest==0x000c131cu);assert(r.m68k_regs.d[0]==0xaaaa0029u&&r.m68k_regs.d[2]==0xbbbb0015u&&r.m68k_regs.d[7]==0xcccc0070u);puts("PASS $8CF8 early");}

static void test_2836(void){
    wf_runtime_t r;wf_r17_post7_ctx_t c;sink_t s;wf_r17_render_ctx_t rc;wf_r17_render_queue_t q;uint32_t d[8];
    init(&r,&c,&s,&rc,&q);
    for(unsigned i=0;i<8u;i++){r.m68k_regs.d[i]=0x11110000u+i;d[i]=r.m68k_regs.d[i];}
    wf_set_be16(&r.actors[0],WF_A_Y,0xff00u);wf_set_be16(&r.actors[1],WF_A_Y,0x0100u);
    q.list[0][0]=&r.actors[0];q.list[0][1]=&r.actors[1];q.count[0]=2u;
    q.list[5][0]=&r.actors[2];q.count[5]=1u;
    assert(wf_r17_owner_2836(&r,&c)==WF_OK);
    assert(s.nrender==3u);assert(s.rendered[0]==&r.actors[2]);assert(s.rendered[1]==&r.actors[1]&&s.rendered[2]==&r.actors[0]);
    for(unsigned i=0;i<6u;i++)assert(r.m68k_regs.d[i]==d[i]);
    assert(r.m68k_regs.d[6]==0x66660001u&&r.m68k_regs.d[7]==0u);
    for(unsigned i=0;i<WF_R17_RENDER_LISTS;i++)assert(q.count[i]==0u);
    /* blocked WP26 join must not consume queues or leak register changes */
    q.list[0][0]=&r.actors[0];q.count[0]=1u;s.block_render=1;r.m68k_regs.d[6]=0xaaaabbbbu;r.m68k_regs.d[7]=0xccccddddu;
    assert(wf_r17_owner_2836(&r,&c)==WF_BLOCKED);assert(q.count[0]==1u);assert(r.m68k_regs.d[6]==0xaaaabbbbu&&r.m68k_regs.d[7]==0xccccddddu);assert(c.blocked_dependency_rom==0x0000d1fcu);
    puts("PASS $2836 WP26 join / D0-D5 preserve / D6-D7 live");
}


static void test_18c4(void){
    wf_runtime_t r;wf_r17_post7_ctx_t c;sink_t s;wf_r17_render_ctx_t rc;wf_r17_render_queue_t q;uint8_t in[WF_ACTOR_SLOTS];
    init(&r,&c,&s,&rc,&q);memset(in,0,sizeof(in));
    /* Credited route clear: immediate RTS, no external tail. */
    assert(wf_r17_owner_18c4(&r,&c,in,0u)==WF_OK);assert(s.n6e34==0u&&s.n9132==0u);
    /* TAG path is an exact tail join to $6E34. */
    r.g.credited_route=1u;r.g.mode_0161=0u;assert(wf_r17_owner_18c4(&r,&c,in,0u)==WF_OK);assert(s.n6e34==1u);
    /* Royal low byte $1C16C5 nonzero returns before payment. */
    r.g.mode_0161=1u;r.g.global_16c4=0x0001u;assert(wf_r17_owner_18c4(&r,&c,in,0u)==WF_OK);assert(s.n9132==0u);
    /* Empty table: payment loop marks slot0; reverse scan 3->0 selects paid index0;
       first inactive reserve actor is slot4. */
    r.g.global_16c4=0u;r.g.config_0066=0x0004u;r.g.credit_count_004f=1u;r.g.credit_remainder_004e=0u;
    assert(wf_r17_owner_18c4(&r,&c,in,0x000fu)==WF_OK);assert(s.n9132==1u);assert(s.paid9132==0u);assert(s.target9132==&r.actors[4]);assert(r.g.royal_slots_1586[0]==0x8000u);assert(r.g.credit_count_004f==0u);assert(r.g.global_15ce==0u);
    /* Existing high-byte bit7 marker skips payment. Exact marker selection is 3->0.
       With actors4..8 active, A0 falls through to $1C0F1C and $1C15CE=$8000. */
    init(&r,&c,&s,&rc,&q);memset(in,0,sizeof(in));r.g.credited_route=1u;r.g.mode_0161=1u;r.g.royal_slots_1586[3]=0x8000u;
    for(unsigned i=4u;i<=8u;i++)wf_set_u8(&r.actors[i],WF_A_ACTIVE,0x80u);
    assert(wf_r17_owner_18c4(&r,&c,in,0u)==WF_OK);assert(s.n9132==1u);assert(s.paid9132==3u);assert(s.target9132==&r.fixed_object0);assert(r.g.global_15ce==0x8000u);assert((uint16_t)r.m68k_regs.d[1]==0xffffu);
    /* Missing $9132 must roll back local payment/table/register mutations. */
    init(&r,&c,&s,&rc,&q);memset(in,0,sizeof(in));r.g.credited_route=1u;r.g.mode_0161=1u;r.g.config_0066=0x0004u;r.g.credit_count_004f=1u;c.royal_9132=0;
    r.m68k_regs.d[0]=0xaaaa5555u;r.m68k_regs.d[3]=0xbbbb6666u;r.m68k_regs.d[4]=0xcccc7777u;
    assert(wf_r17_owner_18c4(&r,&c,in,0x000fu)==WF_BLOCKED);assert(c.blocked_dependency_rom==0x00009132u);assert(r.g.credit_count_004f==1u&&r.g.royal_slots_1586[0]==0u);assert(r.m68k_regs.d[0]==0xaaaa5555u&&r.m68k_regs.d[3]==0xbbbb6666u&&r.m68k_regs.d[4]==0xcccc7777u);
    puts("PASS $18C4 TAG/Royal payment/participant join");
}

static void test_298ec(void){
    wf_runtime_t r;wf_r17_post7_ctx_t c;sink_t s;wf_r17_render_ctx_t rc;wf_r17_render_queue_t q;uint32_t d2,d3,d5,d6,d7;
    init(&r,&c,&s,&rc,&q);c.video_batch=fg;
    r.m68k_regs.d[0]=0xaaaa1111u;r.m68k_regs.d[1]=0xbbbb2222u;r.m68k_regs.d[2]=0xcccc3333u;r.m68k_regs.d[3]=0xdddd4444u;r.m68k_regs.d[4]=0xeeee5555u;r.m68k_regs.d[5]=0x12345678u;r.m68k_regs.d[6]=0x89abcdefu;r.m68k_regs.d[7]=0xfedcba98u;
    d2=r.m68k_regs.d[2];d3=r.m68k_regs.d[3];d5=r.m68k_regs.d[5];d6=r.m68k_regs.d[6];d7=r.m68k_regs.d[7];
    assert(wf_r17_owner_298ec(&r,&c)==WF_OK);assert(r.m68k_regs.d[0]==0xaaaa1111u&&r.m68k_regs.d[1]==0xbbbb2222u&&r.m68k_regs.d[4]==0xeeee5555u);assert(s.nfg==0u);
    r.g.transfer_count_1828=2u;r.g.transfer_queue_1808[0]=1u;r.g.transfer_queue_1808[1]=2u;
    assert(wf_r17_owner_298ec(&r,&c)==WF_OK);assert(s.nfg==36u);assert(s.fg[0].addr==0x0008218au&&s.fg[0].value==0u&&s.fg[0].width==2u);assert(s.fg[18].addr==0x0008218au&&s.fg[18].value==0x02bbu);assert(s.fg[24].addr==0x000821cau&&s.fg[24].value==0x028au);assert(s.fg[30].addr==0x0008220au&&s.fg[30].value==0x029au);
    assert(r.g.transfer_count_1828==0u);for(unsigned i=0;i<16u;i++)assert(r.g.transfer_queue_1808[i]==0u);
    assert(r.m68k_regs.d[0]==0xaaaa0004u);assert(r.m68k_regs.d[1]==0x0000018au);assert(r.m68k_regs.d[4]==0xeeeeffffu);assert(r.m68k_regs.d[2]==d2&&r.m68k_regs.d[3]==d3&&r.m68k_regs.d[5]==d5&&r.m68k_regs.d[6]==d6&&r.m68k_regs.d[7]==d7);
    puts("PASS $298EC");
}

int main(void){test_88a2();test_8710();test_bb14_fallback_lowest_free();test_7366();test_8cf8_early();test_2836();test_18c4();test_298ec();puts("PASS post-$7A00 new owners");return 0;}
