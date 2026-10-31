#include "wf_arcade_clock.h"
#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>

static void exact_periods(void) {
    wf_arcade_clock_t c;
    assert(wf_arcade_clock_init(&c, 0));
    for (unsigned i=0;i<39168u*100u;i++) {
        assert(wf_arcade_clock_field(&c));
        while(wf_arcade_clock_take(&c)) { }
    }
    assert(c.fields==3916800u && c.ticks==3760900u);
    assert(c.pending==0 && c.phase==0 && !c.failed);
}
static void debt_is_preserved(void) {
    wf_arcade_clock_t c;
    assert(wf_arcade_clock_init(&c,39167));
    assert(wf_arcade_clock_field(&c));
    assert(c.pending==1 && c.ticks==0);
    for(unsigned i=1;i<120;i++) assert(wf_arcade_clock_field(&c));
    assert(c.pending==(39167u+120u*37609u)/39168u);
    unsigned owed=c.pending;
    for(unsigned i=0;i<owed;i++) assert(wf_arcade_clock_take(&c));
    assert(!wf_arcade_clock_take(&c));
    assert(c.fields==120 && c.ticks==owed && !c.failed);
}
static void invalid_and_overflow(void) {
    wf_arcade_clock_t c,before;
    memset(&c,0x55,sizeof c); before=c;
    assert(!wf_arcade_clock_init(NULL,0));
    assert(!wf_arcade_clock_init(&c,39168));
    assert(memcmp(&c,&before,sizeof c)==0);
    assert(!wf_arcade_clock_field(NULL));
    assert(!wf_arcade_clock_take(NULL));
    assert(wf_arcade_clock_init(&c,39167));
    c.pending=UINT32_MAX; before=c;
    assert(!wf_arcade_clock_field(&c));
    assert(c.failed && c.pending==before.pending && c.fields==before.fields);
    assert(!wf_arcade_clock_take(&c));
    assert(wf_arcade_clock_init(&c,0)); c.fields=UINT64_MAX;
    assert(!wf_arcade_clock_field(&c) && c.failed);
    assert(wf_arcade_clock_init(&c,0));c.pending=1;c.ticks=UINT64_MAX;
    assert(!wf_arcade_clock_take(&c) && c.pending==1 && c.failed);
}
int main(void) {
    exact_periods(); debt_is_preserved(); invalid_and_overflow();
    puts("PASS clock: 100 rational periods, retained debt, invalid inputs, overflow");
    return 0;
}
