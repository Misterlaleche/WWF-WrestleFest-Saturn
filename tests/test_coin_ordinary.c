#include "wf_r18_frontend_coin.h"
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void reference_coin_enter(wf_r18_frontend_coin_t *,uint16_t,uint32_t);
int reference_coin_pulse_048c(wf_r18_frontend_control_context_t *);

/* The alias view is an unsigned-char view of actual typed objects. Callback
 * mutations through that view are legal C aliases, not casts of an unaligned
 * byte allocation to invented struct objects. It also includes full RAM. */
typedef struct region {
    uint8_t prefix[128];
    wf_r18_frontend_coin_t state;
    wf_r17_m68k_shadow_t regs;
    uint32_t a[7];
    wf_r18_frontend_control_context_t context;
    uint8_t tail[WF_R18_TITLE_RAM_BYTES];
} region_t;
typedef struct observation {
    uint32_t kind,owner,address,value,d[8],a[7],ssp,usp;
    uint16_t sr;
    uint8_t raw[0x2e];
    wf_r18_frontend_coin_t state;
} observation_t;
typedef struct fixture {
    region_t region;
    uint8_t plain_ram[WF_R18_TITLE_RAM_BYTES];
    observation_t observations[8];
    uint32_t input,post_d[3];
    uint16_t post_sr;
    unsigned mode,failure,events,recursive,alias;
    int recursive_status;
    int (*pulse)(wf_r18_frontend_control_context_t *);
} fixture_t;

static unsigned long case_number,entry_cases,pulse_cases,callback_events,alias_cases,recursive_cases;
static const char *phase;
#define CHECK(x) do {if(!(x)){fprintf(stderr,"FAIL %s case=%lu: %s\n",phase,case_number,#x);exit(86);}} while(0)

static int callback(void *,wf_r18_title_event_t *,wf_r18_frontend_control_context_t *);
static int replacement(void *u,wf_r18_title_event_t *e,wf_r18_frontend_control_context_t *c)
{
    return callback(u,e,c);
}

static void mutate(fixture_t *f,wf_r18_frontend_control_context_t *c)
{
    if(f->mode==2u){
        CHECK(f->alias);
        for(unsigned i=0;i<3;i++){
            size_t at=(size_t)((uint8_t *)&f->region.regs.d[i]-(uint8_t *)&f->region);
            memcpy(c->ram_1c0000+at,&f->post_d[i],sizeof f->post_d[i]);
        }
        size_t at=(size_t)((uint8_t *)&c->sr-(uint8_t *)&f->region);
        memcpy(c->ram_1c0000+at,&f->post_sr,sizeof f->post_sr);
    }else{
        for(unsigned i=0;i<3;i++)c->regs->d[i]=f->post_d[i];
        c->sr=f->post_sr;
    }
}

static int callback(void *u,wf_r18_title_event_t *e,wf_r18_frontend_control_context_t *c)
{
    fixture_t *f=u;
    CHECK(c==&f->region.context&&f->events<8u);
    observation_t *o=&f->observations[f->events++];
    unsigned ordinal=f->events;
    o->kind=e->kind;o->owner=e->owner_rom;o->address=e->address;o->value=e->value;
    memcpy(o->d,c->regs->d,sizeof o->d);memcpy(o->a,c->a,sizeof o->a);
    o->ssp=c->ssp;o->usp=c->usp;o->sr=c->sr;
    memcpy(o->raw,c->ram_1c0000+0x4b,sizeof o->raw);
    memcpy(&o->state,&f->region.state,sizeof o->state);
    if(e->kind==WF_R18_CONTROL_READ16){
        CHECK(e->owner_rom==0x48cu&&e->address==0x140020u&&e->value==0u);
        e->value=f->input;
        if(f->mode&&!f->recursive)mutate(f,c);
        if(f->mode==3u)c->effect=replacement;
        if(f->mode==4u&&!f->recursive){
            f->recursive=1;
            f->recursive_status=f->pulse(c);
            f->recursive=0;
        }
    }else{
        CHECK(e->kind==WF_R18_CONTROL_WRITE16&&e->owner_rom==0x500u&&e->address==0x14000cu&&e->value==0x312au);
        if(f->mode&&!f->recursive){
            c->sr^=0x351fu;c->regs->d[1]^=0xa5a500ffu;
            c->ram_1c0000[0x4e]^=3u;
        }
    }
    return f->failure==ordinal;
}

static void initialize(fixture_t *f,unsigned selector,unsigned duration,unsigned release,
                       uint16_t sr,unsigned mode,unsigned alias,unsigned failure,
                       int (*pulse)(wf_r18_frontend_control_context_t *))
{
    memset(f,0,sizeof *f);
    f->pulse=pulse;f->mode=mode;f->alias=alias;f->failure=failure;
    f->post_d[0]=0x89ab0000u|(uint16_t)duration;
    f->post_d[1]=0x1234ab80u|selector;
    f->post_d[2]=0xfedcba00u|(uint8_t)release;
    f->post_sr=sr;
    f->input=0x100u<<selector;
    for(unsigned i=0;i<8;i++)f->region.regs.d[i]=0x76543210u^((uint32_t)i*0x02040811u);
    for(unsigned i=0;i<7;i++)f->region.a[i]=0x12345678u^((uint32_t)i*0x11020408u);
    for(unsigned i=0;i<3;i++)f->region.regs.d[i]=f->post_d[i];
    f->region.regs.valid=1;
    memset(&f->region.state,0xa5,sizeof f->region.state);
    wf_r18_frontend_control_context_t *c=&f->region.context;
    c->ram_1c0000=alias?(uint8_t *)&f->region:f->plain_ram;
    c->ram_bytes=WF_R18_TITLE_RAM_BYTES;c->regs=&f->region.regs;c->a=f->region.a;
    c->ssp=0x1c3bbcu;c->usp=0x1c3fd4u;c->sr=sr;c->effect=callback;c->user=f;
    c->ram_1c0000[0x4b]=2;c->ram_1c0000[0x4d]=1;c->ram_1c0000[0x4f]=9;
    if(mode){
        /* All post-READ values differ, including the selected input bit. */
        c->regs->d[0]^=0x0000ffffu;c->regs->d[1]^=7u;c->regs->d[2]^=0xffu;c->sr^=0xafffu;
    }
}

static void compare(fixture_t *a,fixture_t *b,int sa,int sb)
{
    CHECK(sa==sb&&a->events==b->events&&a->recursive_status==b->recursive_status);
    CHECK(memcmp(a->observations,b->observations,sizeof a->observations)==0);
    CHECK(memcmp(a->plain_ram,b->plain_ram,sizeof a->plain_ram)==0);
    wf_r18_frontend_control_context_t saved_a,saved_b;
    memcpy(&saved_a,&a->region.context,sizeof saved_a);
    memcpy(&saved_b,&b->region.context,sizeof saved_b);
    CHECK(saved_a.ram_1c0000==(a->alias?(uint8_t *)&a->region:a->plain_ram));
    CHECK(saved_b.ram_1c0000==(b->alias?(uint8_t *)&b->region:b->plain_ram));
    CHECK(saved_a.regs==&a->region.regs&&saved_b.regs==&b->region.regs);
    CHECK(saved_a.a==a->region.a&&saved_b.a==b->region.a&&saved_a.user==a&&saved_b.user==b);
    CHECK(saved_a.effect==saved_b.effect);
    /* Normalize only fixture addresses; compare all remaining object/RAM
     * bytes, including state, register upper bits, padding and guard tail. */
    a->region.context.ram_1c0000=b->region.context.ram_1c0000=NULL;
    a->region.context.regs=b->region.context.regs=NULL;
    a->region.context.a=b->region.context.a=NULL;
    a->region.context.user=b->region.context.user=NULL;
    CHECK(memcmp(&a->region,&b->region,sizeof a->region)==0);
    memcpy(&a->region.context,&saved_a,sizeof saved_a);
    memcpy(&b->region.context,&saved_b,sizeof saved_b);
    callback_events+=a->events;
}

static void pulse(unsigned selector,unsigned duration,unsigned release,uint16_t sr,
                  unsigned mode,unsigned alias,unsigned failure,unsigned active)
{
    static fixture_t a,b;
    initialize(&a,selector,duration,release,sr,mode,alias,failure,reference_coin_pulse_048c);
    initialize(&b,selector,duration,release,sr,mode,alias,failure,wf_r18_frontend_coin_pulse_048c);
    if(active)a.input=b.input=0;
    int sa=a.pulse(&a.region.context),sb=b.pulse(&b.region.context);
    compare(&a,&b,sa,sb);
    pulse_cases++;case_number++;alias_cases+=alias;recursive_cases+=mode==4u;
}

static void entry(void)
{
    struct guarded {uint32_t before[4];wf_r18_frontend_coin_t state;uint32_t after[4];} a,b,expected;
    _Static_assert(sizeof(wf_r18_frontend_coin_t)==28,"Native coin state must exercise all28bytes");
    phase="enter all SR/full object";
    reference_coin_enter(NULL,0xffffu,0xffffffffu);wf_r18_frontend_coin_enter(NULL,0xffffu,0xffffffffu);
    for(unsigned sr=0;sr<=0xffffu;sr++){
        uint8_t *bytes=(uint8_t *)&a;
        for(size_t i=0;i<sizeof a;i++)bytes[i]=(uint8_t)((sr^(sr>>8))+(i*37u));
        memcpy(&b,&a,sizeof a);memcpy(&expected,&a,sizeof a);
        uint32_t pc=((uint32_t)sr<<16)^0xa5c30079u;
        memset(&expected.state,0,sizeof expected.state);
        expected.state.pc=0x434u;expected.state.return_sr=(uint16_t)sr;
        expected.state.return_pc=pc;expected.state.frame_valid=1;
        reference_coin_enter(&a.state,(uint16_t)sr,pc);
        wf_r18_frontend_coin_enter(&b.state,(uint16_t)sr,pc);
        CHECK(memcmp(&a,&expected,sizeof a)==0&&memcmp(&b,&expected,sizeof b)==0);
        entry_cases++;case_number++;
    }
}

int main(void)
{
    entry();
    phase="all SR/all eight live selectors";
    for(unsigned sr=0;sr<=0xffffu;sr++)for(unsigned selector=0;selector<8u;selector++)
        pulse(selector,0,10,(uint16_t)sr,0,0,0,0);
    static const unsigned durations[]={0,1,0x35f,0x360,0x7fff,0x8000,0xffff};
    static const unsigned releases[]={0,1,3,4,9,10,11,127,128,137,138,255};
    static const uint16_t flags[]={0,1,2,4,8,16,31,0x2000,0xa71f,0xffff};
    phase="duration/release/flags/mutation/failure boundaries";
    for(unsigned selector=0;selector<8u;selector++)
    for(size_t d=0;d<sizeof durations/sizeof durations[0];d++)
    for(size_t r=0;r<sizeof releases/sizeof releases[0];r++)
    for(size_t s=0;s<sizeof flags/sizeof flags[0];s++)
    for(unsigned active=0;active<2u;active++){
        unsigned mode=(unsigned)((d+r+s+active)%4u);
        unsigned alias=mode==2u||(s&1u);
        unsigned failure=(unsigned)((d+2u*r+s)%3u);
        pulse(selector,durations[d],releases[r],flags[s],mode,alias,failure,active);
    }
    phase="aliased callback writes/all flags";
    for(unsigned sr=0;sr<=0xffffu;sr++)
        pulse(sr&7u,0,10,(uint16_t)sr,2,1,0,0);
    phase="callback replacement and helper reentry";
    for(unsigned selector=0;selector<8u;selector++)
    for(size_t s=0;s<sizeof flags/sizeof flags[0];s++)
    for(unsigned failure=0;failure<3u;failure++){
        pulse(selector,0,10,flags[s],3,1,failure,0);
        pulse(selector,0,10,flags[s],4,1,failure,0);
    }
    printf("PASS ordinary coin: %lu entry cases, %lu pulse cases, %lu ordered callbacks, %lu alias cases, %lu reentrant cases\n",
           entry_cases,pulse_cases,callback_events,alias_cases,recursive_cases);
    printf("{\"entry_cases\":%lu,\"pulse_cases\":%lu,\"callback_events\":%lu,\"alias_cases\":%lu,\"reentrant_cases\":%lu}\n",
           entry_cases,pulse_cases,callback_events,alias_cases,recursive_cases);
    return 0;
}
