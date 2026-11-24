#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include "wf_jo_clock.h"
int main(void) {
    wf_jo_clock_t c;
    wf_jo_clock_start(&c, 10);
    assert(wf_jo_clock_advance(&c, 10) == 1 && c.pending == 0);
    assert(wf_jo_clock_advance(&c, 11) == 1 && c.pending == 0);
    assert(wf_jo_clock_advance(&c, 12) == 1 && c.pending == 1);
    assert(wf_jo_clock_take(&c) == 1 && c.pending == 0);
    assert(wf_jo_clock_take(&c) == 0);
    wf_jo_clock_start(&c, 0);
    for (unsigned i=1; i<=39168; ++i) assert(wf_jo_clock_advance(&c, i));
    assert(c.pending == 37609 && c.remainder == 0);
    wf_jo_clock_start(&c, UINT32_MAX-1);
    assert(wf_jo_clock_advance(&c, 1) && c.pending == 2);
    wf_jo_clock_start(&c, 0);
    assert(wf_jo_clock_advance(&c, 100));
    assert(c.pending == 96); /* Late frames retain simulation debt. */
    for (unsigned i=0; i<96; ++i) assert(wf_jo_clock_take(&c));
    assert(!wf_jo_clock_take(&c));
    c.pending = UINT32_MAX;
    assert(!wf_jo_clock_advance(&c, 102));
    puts("clock: exact rational cadence, retained debt, wrap and overflow PASS");
    return 0;
}
