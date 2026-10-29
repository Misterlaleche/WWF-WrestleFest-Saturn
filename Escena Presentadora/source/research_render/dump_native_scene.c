#include <stdio.h>
#include <stdlib.h>
#include "wp02_scene.h"

static void emit(unsigned index, const wp02_scene *s, unsigned actor_index, const wp02_actor *a)
{
    printf("%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u\n", index, s->ticks,
        s->presenter.state & 255u, s->done, s->names_visible, actor_index,
        a->active, a->type, a->selector, a->x, a->y, a->z);
}

int main(int argc,char **argv)
{
    wp02_scene s;
    uint8_t identities[4]={0,6,9,1};
    if(argc==5) for(unsigned i=0;i<4;i++)identities[i]=(uint8_t)strtoul(argv[i+1],0,0);
    if(!wp02_init(&s,identities,0xceb7c58eu))return 1;
    puts("index,ticks,state,done,names,actor,active,type,selector,x,y,z");
    for(unsigned i=0;i<3000;i++) {
        wp02_tick(&s);
        emit(i,&s,0,&s.referee);
        emit(i,&s,1,&s.presenter);
        emit(i,&s,2,&s.wrestlers[1]);
        emit(i,&s,3,&s.wrestlers[3]);
        emit(i,&s,4,&s.wrestlers[0]);
        emit(i,&s,5,&s.wrestlers[2]);
        if(s.error)return 2;
        if(s.done)break;
    }
    return 0;
}
