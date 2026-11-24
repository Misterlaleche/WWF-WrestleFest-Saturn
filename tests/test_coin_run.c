/* Whole IRQ434 must have the same observable boundaries as repeated step().
 * In particular a rejected MMIO event must not be skipped, duplicated on
 * resume, or leave partially applied credit/RAM/register changes behind.
 * tests/test_coin_irq_rom.py checks the independent original-ROM contract. */
#include "wf_r18_frontend_coin.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

/* Weak only so the preimplementation test reports a missing behavior rather
 * than failing to link. Production code never refers to this declaration. */
extern int wf_r18_frontend_coin_run(wf_r18_frontend_coin_t *,
    wf_r18_frontend_control_context_t *) __attribute__((weak));

typedef struct observed {
    wf_r18_title_event_t event;
    wf_r18_frontend_coin_t state;
    uint32_t d[8],a[7],ssp,usp,ram_hash;
    uint16_t sr;
    uint8_t rejected;
} observed_t;
typedef struct fixture {
    uint8_t ram[0x4000];
    wf_r17_m68k_shadow_t regs;
    uint32_t a[7];
    wf_r18_frontend_coin_t state;
    wf_r18_frontend_control_context_t context;
    uint16_t input[8];
    unsigned reads,events,fail_event;
    unsigned mutate_read;
    uint16_t post_read_sr;
    observed_t log[16];
} fixture_t;
static uint32_t seed=0x43404f2u;
static unsigned comparisons,resumes;
static const void *watched_d,*watched_credit;
static unsigned rollback_copies;
void *__real_memcpy(void *,const void *,size_t);
void *__wrap_memcpy(void *to,const void *from,size_t bytes) {
    if((from==watched_d&&bytes==12u)||(from==watched_credit&&bytes==6u))++rollback_copies;
    return __real_memcpy(to,from,bytes);
}
static uint32_t random32(void) {
    seed^=seed<<13;seed^=seed>>17;seed^=seed<<5;return seed;
}
static uint32_t hash_ram(const uint8_t *ram) {
    uint32_t h=2166136261u;
    for(unsigned i=0;i<0x4000u;i++)h=(h^ram[i])*16777619u;
    return h;
}
static int effect(void *user,wf_r18_title_event_t *event,
                  wf_r18_frontend_control_context_t *context) {
    fixture_t *f=user;
    assert(context==&f->context);
    assert(f->events<16u);
    observed_t *o=&f->log[f->events++];
    memset(o,0,sizeof *o);
    o->event=*event;o->state=f->state;
    memcpy(o->d,context->regs->d,sizeof o->d);
    memcpy(o->a,context->a,sizeof o->a);
    o->ssp=context->ssp;o->usp=context->usp;o->sr=context->sr;
    o->ram_hash=hash_ram(context->ram_1c0000);
    if(f->events==f->fail_event) { o->rejected=1;return 1; }
    if(event->kind==WF_R18_CONTROL_READ16) {
        assert(event->address==0x140020u&&f->reads<8u);
        event->value=f->input[f->reads++];
        o->event.value=event->value;
        if(f->mutate_read&&event->owner_rom==0x48cu) {
            /* Adversarial read seam: rollback must use this state, not the
             * earlier helper entry or the later CMP flags before credit. */
            static const uint8_t credit[6]={2,17,0x12,0x34,0xfe,0xff};
            context->regs->d[0]=0x12340001u;
            context->regs->d[1]=0x34560000u;
            context->regs->d[2]=0x789a0004u;
            context->sr=f->post_read_sr;
            memcpy(context->ram_1c0000+0x4e,credit,sizeof credit);
        }
    } else {
        assert(event->kind==WF_R18_CONTROL_WRITE16);
        assert((event->address==0x140016u&&event->owner_rom==0x438u&&event->value==0u)||
               (event->address==0x140002u&&event->owner_rom==0x480u&&event->value==0u)||
               (event->address==0x14000cu&&event->owner_rom==0x500u&&event->value==0x312au));
    }
    return 0;
}
static void bind(fixture_t *f) {
    f->context.ram_1c0000=f->ram;f->context.regs=&f->regs;
    f->context.a=f->a;f->context.effect=effect;f->context.user=f;
}
static void copy(fixture_t *to,const fixture_t *from) {
    memcpy(to,from,sizeof *to);bind(to);
}
static void put16(uint8_t *p,unsigned value) {
    p[0]=(uint8_t)(value>>8);p[1]=(uint8_t)value;
}
static void init(fixture_t *f) {
    memset(f,0,sizeof *f);
    for(unsigned i=0;i<sizeof f->ram;i++)f->ram[i]=(uint8_t)random32();
    for(unsigned i=0;i<8u;i++)f->regs.d[i]=random32();
    for(unsigned i=0;i<7u;i++)f->a[i]=random32();
    f->regs.valid=1;
    f->context.ram_bytes=sizeof f->ram;
    f->context.ssp=0x1c3bceu;f->context.usp=0x1c3fd4u;
    uint16_t interrupted_sr=(uint16_t)random32();
    f->context.sr=(uint16_t)((interrupted_sr&0x1fu)|0x2200u);
    wf_r18_frontend_coin_enter(&f->state,interrupted_sr,random32()&0x7fffeu);
    f->state.blocked_dependency=random32(); /* Success must not erase it. */
    for(unsigned i=0;i<8u;i++)f->input[i]=(uint16_t)random32();
    bind(f);
}
static int reference(fixture_t *f) {
    int status=0;unsigned steps=0;
    while(status==WF_R18_CONTROL_MORE) {
        assert(++steps<20u);
        status=wf_r18_frontend_coin_step(&f->state,&f->context);
    }
    return status;
}
static void equal(const fixture_t *a,const fixture_t *b,int ar,int br) {
    assert(ar==br);
    assert(memcmp(a->ram,b->ram,sizeof a->ram)==0);
    assert(memcmp(&a->regs,&b->regs,sizeof a->regs)==0);
    assert(memcmp(a->a,b->a,sizeof a->a)==0);
    assert(memcmp(&a->state,&b->state,sizeof a->state)==0);
    assert(a->context.ssp==b->context.ssp&&a->context.usp==b->context.usp);
    assert(a->context.sr==b->context.sr);
    assert(a->events==b->events&&a->reads==b->reads);
    assert(memcmp(a->log,b->log,a->events*sizeof a->log[0])==0);
    comparisons++;
}
static void compare(fixture_t *initial,unsigned fail_event) {
    fixture_t a,b;
    copy(&a,initial);copy(&b,initial);
    a.fail_event=b.fail_event=fail_event;
    int ar=reference(&a),br=wf_r18_frontend_coin_run(&b.state,&b.context);
    equal(&a,&b,ar,br);
    if(ar==WF_R18_CONTROL_BLOCKED&&fail_event) {
        a.fail_event=b.fail_event=0;
        ar=reference(&a);br=wf_r18_frontend_coin_run(&b.state,&b.context);
        equal(&a,&b,ar,br);assert(ar==WF_R18_CONTROL_RTE);resumes++;
    }
}
static void case_and_boundaries(fixture_t *f) {
    /* Every public PC is a legal resume boundary, including after a previous
     * accepted write; restarting at434 would duplicate that write. */
    fixture_t partial;copy(&partial,f);
    for(unsigned i=0;i<20u;i++) {
        compare(&partial,0);
        if(wf_r18_frontend_coin_step(&partial.state,&partial.context)!=WF_R18_CONTROL_MORE)return;
    }
    assert(!"step failed to terminate");
}
static void invalid_contexts(void) {
    fixture_t a,b;
    assert(wf_r18_frontend_coin_run(NULL,NULL)==WF_R18_CONTROL_BAD_CONTEXT);
    for(unsigned which=0;which<10u;which++) {
        init(&a);copy(&b,&a);
        switch(which) {
        case 0:a.context.ram_1c0000=b.context.ram_1c0000=NULL;break;
        case 1:a.context.ram_bytes=b.context.ram_bytes=0x3fffu;break;
        case 2:a.context.regs=b.context.regs=NULL;break;
        case 3:a.regs.valid=b.regs.valid=0;break;
        case 4:a.context.a=b.context.a=NULL;break;
        case 5:a.context.effect=b.context.effect=NULL;break;
        case 6:a.context.sr=b.context.sr=0;break;
        case 7:a.state.frame_valid=b.state.frame_valid=0;break;
        case 8:a.state.pc=b.state.pc=0x486u;break;
        case 9:a.state.pc=b.state.pc=0x48au;a.state.frame_valid=b.state.frame_valid=0;break;
        }
        int ar=reference(&a),br=wf_r18_frontend_coin_run(&b.state,&b.context);
        equal(&a,&b,ar,br);assert(ar==WF_R18_CONTROL_BAD_CONTEXT);
    }
    init(&a);a.state.pc=0x123456u;compare(&a,0);
}
static void pulse_rollback_contract(void) {
    static const uint8_t expected_credit[6]={2,17,0x12,0x34,0xfe,0xff};
    fixture_t f;
    for(unsigned flags=0;flags<32u;flags++) {
        init(&f);f.state.pc=0x450u;
        f.mutate_read=1;f.post_read_sr=(uint16_t)(0xa700u|flags);
        f.input[0]=0xffffu;f.fail_event=2;
        int result=wf_r18_frontend_coin_pulse_048c(&f.context);
        assert(result==WF_R18_CONTROL_BLOCKED&&f.events==2u);
        assert(f.regs.d[0]==0x12340001u&&f.regs.d[1]==0x34560000u&&f.regs.d[2]==0x789a0004u);
        assert(f.context.sr==f.post_read_sr);
        assert(memcmp(f.ram+0x4e,expected_credit,sizeof expected_credit)==0);
        assert(f.state.pc==0x450u);
        /* Retrying the same helper must reread the live port before its
         * sound write, preserving the previously blocked owner boundary. */
        f.fail_event=0;f.input[1]=0xffffu;
        assert(wf_r18_frontend_coin_pulse_048c(&f.context)==0);
        assert(f.events==4u&&f.log[2].event.owner_rom==0x48cu&&f.log[3].event.owner_rom==0x500u);
    }
}
static void idle_pulse_has_no_rollback_copy(void) {
    fixture_t f;init(&f);f.state.pc=0x450u;f.input[0]=0xffffu;
    f.regs.d[0]=0x12340000u;f.regs.d[1]=0x34560000u;f.regs.d[2]=0x789a000au;
    watched_d=f.regs.d;watched_credit=f.ram+0x4e;rollback_copies=0;
    assert(wf_r18_frontend_coin_pulse_048c(&f.context)==0);
    watched_d=watched_credit=NULL;
    if(rollback_copies)fprintf(stderr,"Idle pulse made %u unnecessary rollback copies\n",rollback_copies);
    assert(rollback_copies==0u);
}
int main(void) {
    if(!wf_r18_frontend_coin_run) {
        fputs("FAIL: IRQ434 whole runner is absent\n",stderr);return 1;
    }
    static const unsigned duration[]={0,1,0x35f,0x360,0xffff};
    static const unsigned release[]={0,3,4,9,10,127,128,255};
    static const unsigned service[]={0,30,31,127,128,255};
    fixture_t f;
    /* Boundary cross-product catches signed/unsigned threshold changes and
     * byte/word wrap in the real coin/service helpers. */
    for(unsigned d=0;d<sizeof duration/sizeof duration[0];d++)
    for(unsigned r=0;r<sizeof release/sizeof release[0];r++)
    for(unsigned s=0;s<sizeof service/sizeof service[0];s++) {
        init(&f);put16(f.ram+0x54,duration[d]);put16(f.ram+0x78,duration[d]);
        f.ram[0x10]=f.ram[0x11]=(uint8_t)release[r];f.ram[0x56]=(uint8_t)service[s];
        for(unsigned i=0;i<8u;i++)f.input[i]=0xffffu;
        for(unsigned fail=0;fail<=7u;fail++)compare(&f,fail);
        if(d==1u&&r==2u)case_and_boundaries(&f);
    }
    /* Distinct values per read expose accidental MMIO coalescing. */
    for(unsigned i=0;i<512u;i++) {
        init(&f);compare(&f,0);
        compare(&f,1u+(i%7u));
        if(i<16u)case_and_boundaries(&f);
    }
    invalid_contexts();
    pulse_rollback_contract();
    idle_pulse_has_no_rollback_copy();
    printf("coin run/step: %u comparisons, %u blocked resumes, all RAM/D/A/SR/SSP/USP and ordered MMIO equal\n",comparisons,resumes);
    return 0;
}
