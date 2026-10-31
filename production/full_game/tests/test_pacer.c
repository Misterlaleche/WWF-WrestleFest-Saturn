#include "wf_pacer.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
int main(void){
 wf_pacer_t p;assert(wf_pacer_init(&p,100,123));
 assert(wf_pacer_account(&p,120));assert(wf_pacer_take(&p));
 wf_arcade_clock_t before=p.clock;
 assert(wf_pacer_load_begin(&p,120));assert(wf_pacer_load_begin(&p,130));
 assert(wf_pacer_load_end(&p,150));assert(wf_pacer_load_end(&p,180));
 assert(!memcmp(&before,&p.clock,sizeof before)&&p.load_fields==60&&p.last_field==180);
 assert(wf_pacer_account(&p,200));
 assert(p.clock.fields==40&&p.clock.ticks+p.clock.pending==(123+40*37609)/39168);
 assert(wf_pacer_init(&p,0xfffffff0,0));assert(wf_pacer_account(&p,10));assert(p.clock.fields==26);
 assert(!wf_pacer_load_end(&p,10)&&p.clock.failed);
 assert(wf_pacer_init(&p,0,0));assert(wf_pacer_load_begin(&p,10));
 assert(!wf_pacer_take(&p)&&p.clock.failed);
 assert(!wf_pacer_init(NULL,0,0));assert(!wf_pacer_init(&p,0,39168));
 puts("PASS pacer: active fields, nested loads, retained phase/debt, wrap, invalid waits");
}
