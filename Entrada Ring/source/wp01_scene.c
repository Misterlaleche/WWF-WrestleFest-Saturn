#include "wp01_scene.h"
#include <string.h>

uint32_t wp01_rng_21b4(uint32_t *state,uint32_t registers,uint32_t timer) {
    uint32_t value=*state+registers+timer;
    unsigned rotation=value&7U;
    if(rotation)value=(value>>rotation)|(value<<(32U-rotation));
    *state=value;
    return value;
}

static void screen_position(wp01_scene_actor_t *a) {
    a->sx=(int16_t)(a->world_x-320);
    a->sy=(int16_t)((a->world_y>>16)+a->base_z-512+a->correction_y);
    a->anchor_x=a->sx;
    a->anchor_y=(int16_t)(232-a->sy);
}

static void spawn_actor(wp01_scene_actor_t *a,unsigned side) {
    a->type=(uint16_t)(0x40U+a->internal_id);
    if(a->type==0x4B)a->type=0x4A;
    a->world_x=(int16_t)(448+48*side);
    a->world_y=304L<<16;
    a->base_z=256;
    a->counter=0;a->phase=0;a->selector=0;a->correction_y=0;
    a->visible=1;
}

static uint32_t random_word(wp01_scene_t *s,unsigned actor) {
    if(s->rng_hook)return s->rng_hook(s->rng_context,actor,s->movement_updates);
    /* The arcade routine mixes its caller's live D1..D7 registers. A native
     * scene has no such register state. Keep the exact add/rotate arithmetic
     * while supplying explicit scene entropy. Trace replay uses the hook.
     * Movement, eligible types, branch condition and call count are exact. */
    uint32_t entropy=(uint32_t)s->movement_updates+
        ((uint32_t)s->actor[actor].type<<16)+(uint32_t)actor;
    return wp01_rng_21b4(&s->rng_state,entropy,0);
}

static void actor_update(wp01_scene_t *s,unsigned index) {
    wp01_scene_actor_t *a=&s->actor[index];
    a->world_y-=0x8000L; /* $2208 speed8 angle$80: (0,-32768)16.16. */
    if((a->world_y>>16)<144)return;
    a->selector=(uint16_t)(a->phase&255U); /* ROM$8136={0,1,2,3,4,5,6,7}. */
    a->correction_y=(int16_t)((a->type==0x41&&a->selector>=4)?48:0);
    ++a->counter;
    if(a->counter>=12) {
        a->counter=0;
        a->phase=(uint16_t)(a->phase+0x0101U);
        if((a->phase&0xFF00U)>=0x0400U) {
            a->phase=0;
            if(a->type==0x40||a->type==0x41||a->type==0x47||a->type==0x49) {
                if(!(random_word(s,index)&1U))a->phase=4;
            }
        }
    }
    screen_position(a);
}

static void light_update(wp01_scene_t *s) {
    wp01_scene_light_t *l=&s->light;
    l->x=152;
    l->y=(int16_t)(487-(s->actor[1].world_y>>16));
    l->selector=l->phase?0xFFFFU:0U;
    l->visible=(uint8_t)(l->selector!=0xFFFFU);
    ++l->counter;
    if(l->counter>(l->phase?1U:2U)) {
        l->counter=0;l->phase^=1U;
    }
}

void wp01_scene_init(wp01_scene_t *s,unsigned id0,unsigned id1,uint32_t seed) {
    memset(s,0,sizeof(*s));
    /* Defensive bounds do not select a random pair; caller owns selection. */
    if(id0>11)id0=0;
    if(id1>11)id1=7;
    if(id0==0) {unsigned t=id0;id0=id1;id1=t;}
    if(id1==9) {unsigned t=id0;id0=id1;id1=t;}
    s->actor[0].internal_id=(uint16_t)id0;
    s->actor[1].internal_id=(uint16_t)id1;
    s->actor[0].type=(uint16_t)id0;s->actor[1].type=(uint16_t)id1;
    s->rng_state=seed;s->wipe_tile=0x4C5;
}

void wp01_scene_set_rng_hook(wp01_scene_t *s,wp01_scene_rng_hook_t hook,void *context) {
    s->rng_hook=hook;s->rng_context=context;
}

void wp01_scene_step(wp01_scene_t *s) {
    if(s->done)return;
    if(s->frame==401) {s->done=1;return;}
    ++s->frame;
    if(s->frame<24) {
        s->wipe_tile=(int16_t)(0x4C5-s->frame/3U);
        return;
    }
    if(s->frame<344) {
        if(s->frame==24) {
            spawn_actor(&s->actor[0],0);spawn_actor(&s->actor[1],1);
        }
        s->wipe_tile=WP01_WIPE_NONE;
        ++s->movement_updates;
        actor_update(s,0);actor_update(s,1);light_update(s);
        return;
    }
    if(s->frame==344) {
        /* $7F0E first actor reaches Y<144 and blocks inside$8F50. The
         * second actor, screen fields and light stay at the prior update
         * throughout the24-frame wipe and33-frameVBlank hold. */
        ++s->movement_updates;
        s->actor[0].world_y-=0x8000L;
    }
    if(s->frame<368) {
        s->wipe_tile=(int16_t)(0x4BE + (s->frame-344U)/3U);
        return;
    }
    s->wipe_tile=0x4C5;
    if(s->frame==401) {
        /* Fade returns inside first actor callback; finish the same
         * iteration, then the caller exits on global$1C0080!=0. */
        screen_position(&s->actor[0]);
        s->actor[1].world_y-=0x8000L;
        screen_position(&s->actor[1]);
        light_update(s);
    }
}
