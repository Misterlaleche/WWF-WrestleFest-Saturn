/* Native SH-2 compatible model of the paid TAG $A654 loop only.
   Verified against execution of supplied 68000 bytes, not WIP timing. */
#include "wp02_scene.h"
#include <string.h>
#include "wp02_rom_tables.h"

uint32_t wp02_rng_step(uint32_t state, const uint32_t mix[7], uint32_t global80)
{
    unsigned i, shift;
    for (i=0;i<7;i++) state += mix[i];
    state += global80;
    shift=state&7u;
    return shift ? (state>>shift)|(state<<(32u-shift)) : state;
}

int wp02_init(wp02_scene *s,const uint8_t identities[4],uint32_t seed)
{
    static const uint16_t xy[4][2]={{0x240,0x150},{0x220,0x160},{0x2c0,0x150},{0x2e0,0x160}};
    unsigned i,j;
    if (!s || !identities) return 0;
    memset(s,0,sizeof(*s));
    for (i=0;i<4;i++) {
        if (identities[i]>=12u) { s->error=1;return 0; }
        for (j=0;j<i;j++) if (identities[i]==identities[j]) { s->error=1;return 0; }
    }
    s->rng=seed;
    /* D-register context of this isolated paid TAG scene, confirmed in the
       original display loop including the original sprite composer. These
       are explicit restorable fields, not a claimed hardware cold boot. */
    s->rng_mix[3]=6u; s->rng_mix[5]=0x001c1610u;
    for (i=0;i<4;i++) {
        wp02_actor *a=&s->wrestlers[i];
        a->active=0x8000u;a->type=identities[i];
        a->x=(uint32_t)xy[i][0]<<16;a->y=(uint32_t)xy[i][1]<<16;
        a->z=0x01400000u;a->flags33=i>=2u?0x80u:0u;
    }
    s->referee.active=0x8000u;s->referee.type=12u;
    s->referee.x=0x03000000u;s->referee.y=0x01980000u;
    s->referee.z=0x01400000u;
    s->names_visible=1;
    return 1;
}

static void sound(wp02_scene *s,uint32_t command)
{
    unsigned n=s->sound_count;
    if (n>=WP02_SOUND_CAPACITY) {s->error=2;return;}
    s->raw_commands[n]=command;
    s->effective_commands[n]=command|0x3100u; /* actual $2052 bus submission */
    s->sound_count=n+1;
}
static unsigned enter(wp02_actor *p)
{
    unsigned fresh=(p->state&0x8000u)==0u;
    p->state|=0x8000u;
    return fresh;
}
static void next_state(wp02_actor *p)
{ p->state=((p->state&255u)+1u)&255u; }
static unsigned logical(unsigned state)
{
    if (state==4u||state==5u) return 0;
    if (state==8u||state==9u) return 1;
    if (state==13u||state==14u) return 2;
    return 3;
}
static wp02_actor *participant(wp02_scene *s,unsigned n)
{
    wp02_actor *a=&s->wrestlers[n];
    return (a->active&0x8000u)?a:&s->wrestlers[4u+(n&1u)];
}
static void opening(wp02_scene *s)
{
    wp02_actor *p=&s->presenter;
    unsigned st=p->state&255u;
    if (enter(p)) {
        p->active=0x8001;p->type=31;p->x=0x02800000;
        p->y=0x01780000;p->z=0x01400000;p->counter=0;
    }
    p->selector=2;
    p->counter=(p->counter+1u)&65535u;
    if (p->counter>(st==0u?128u:32u)) {
        next_state(p);
        if (st==0u) s->names_visible=0; /* actual $9A42 composition clear */
    }
}
static void transition(wp02_actor *p)
{
    unsigned limit;
    if (enter(p)) p->counter=p->phase=0;
    p->selector=2u+p->phase;
    limit=p->phase?32u:16u;
    p->counter=(p->counter+1u)&65535u;
    if (p->counter>limit) {
        p->counter=0;p->phase++;
        if (p->phase>=2u) next_state(p);
    }
}
static void speaking(wp02_scene *s)
{
    wp02_actor *p=&s->presenter;
    unsigned st=p->state&255u,limit,command=0;
    if (enter(p)) {
        p->counter=p->phase=p->counter1e=0;
        if (st==2u) command=0x317b;
        else if (st==6u||st==15u) command=0x317c;
        else if (st==11u) command=0x317d;
        else {
            wp02_actor *a=participant(s,logical(st));
            command=0x3100u|identity_voices[a->type];
            s->group_flag=(command&255u)==0x73u?0x8000u:0u;
        }
        sound(s,command);
    }
    /* ROM writes selector BEFORE its countdown/toggle. */
    p->selector=(p->selector&0xff00u)|(p->phase&255u);
    p->counter=(p->counter-1u)&65535u;
    if (p->counter&0x8000u) {
        s->rng=wp02_rng_step(s->rng,s->rng_mix,s->rng_global80);
        p->counter=6u+(s->rng&15u);
        p->phase^=1u;
    }
    p->counter1e=(p->counter1e+1u)&65535u;
    if (st==2u) limit=160u;
    else if (st==6u||st==15u) limit=20u;
    else if (st==11u) limit=40u;
    else limit=actor_windows[participant(s,logical(st))->type];
    if (p->counter1e>limit) next_state(p);
}
static void activation(wp02_scene *s)
{
    wp02_actor *p=&s->presenter;
    unsigned st=p->state&255u;
    if (enter(p)) {
        wp02_actor *a=participant(s,logical(st));
        a->state=(a->state&0xff00u)|1u;
        sound(s,a->type<5u?0x3032u:0x3031u);
        if (a->type==10u) { /* actual $A878..A886 paired-actor write */
            wp02_actor *next=a+1;
            next->state=(next->state&0xff00u)|1u;
        }
        p->counter=0;
    }
    p->selector=12u; /* A0 = presenter, NOT A2 = announced wrestler */
    p->counter=(p->counter+1u)&65535u;
    if (p->counter>=64u) {
        if (s->group_flag) p->state=(p->state&0xff00u)|((p->state+4u)&255u);
        next_state(p);
    }
}
static void exit_program(wp02_actor *p)
{
    unsigned st=p->state&255u;
    if (enter(p)) p->counter=p->phase=0;
    if (st==21u) {p->y+=0x8000u;p->selector=8u+p->phase;}
    else p->selector=(st==22u?4u:13u)+p->phase;
    p->counter++;
    if (p->counter>=12u) {
        p->counter=0;p->phase++;
        if (p->phase>=4u) {
            if (st==21u) p->phase=0;
            else next_state(p);
        }
    }
    if (st==21u&&(p->y>>16)>=0x1a7u) next_state(p);
}
static void wrestler_step(wp02_actor *a)
{
    unsigned id=a->type;
    if (!(a->active&0x8000u)) return;
    if (!(a->state&255u)) {
        a->selector=(a->flags33&0x80u)?0u:0x8000u;
        a->counter=a->phase=0;
        return;
    }
    a->selector=program_selectors[id][a->phase];
    a->counter=(a->counter+1u)&65535u;
    if (a->counter>program_thresholds[id][a->phase]) {
        a->counter=0;a->phase++;
        if (a->phase>=program_counts[id]) a->state=0;
    }
}
int wp02_tick(wp02_scene *s)
{
    unsigned st,i;
    if (!s || s->done || s->error) return 0;
    s->sound_count=0;
    if (s->terminal_pending) {
        /* $A78E: clear only active/status word. Do not enter gameplay. */
        s->presenter.active=0;s->done=1;return 0;
    }
    if (s->ticks==0u) sound(s,0x3132u);
    st=s->presenter.state&255u;
    switch (st) {
    case 0:case 3:case 19:opening(s);break;
    case 1:case 20:transition(&s->presenter);break;
    case 2:case 4:case 6:case 8:case 11:case 13:case 15:case 17:speaking(s);break;
    case 5:case 9:case 14:case 18:activation(s);break;
    case 7:case 10:case 12:case 16:next_state(&s->presenter);break;
    case 21:case 22:case 23:exit_program(&s->presenter);break;
    default:s->error=3;return 0;
    }
    /* The ROM tests the resulting state after callback, so boundary frames
       such as $0A->$0B already acquire the mirror bit. */
    if ((s->presenter.state&255u)>=11u) s->presenter.selector|=0x8000u;
    for (i=0;i<WP02_WRESTLER_SLOTS;i++) wrestler_step(&s->wrestlers[i]);
    /* $2836's original depth sort makes logical primary slot2 the final
       emitted wrestler at these fixed anchors. Its descriptor interpreter
       leaves D6 for the next shared-RNG call. Extracted native composer
       results: Jake gesture89..91 =>0; Slaughter idle=>FFFF; others=>1C1610.
       This is register-context preservation, not an alternative visual RNG. */
    {
        const wp02_actor *last=&s->wrestlers[2];
        unsigned pose=last->selector&0x7fffu;
        s->rng_mix[5]=(last->type==2u&&pose>=89u&&pose<=91u)?0u:
                      ((last->type==6u&&pose==0u)?0xffffu:0x001c1610u);
    }
    s->ticks++;
    if ((s->presenter.state&255u)>=24u) s->terminal_pending=1;
    return 1;
}
