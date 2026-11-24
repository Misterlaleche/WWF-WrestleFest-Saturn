#include "wf_jo_clock.h"
void wf_jo_clock_start(wf_jo_clock_t *c, uint32_t f) { *c=(wf_jo_clock_t){f,0,0,0}; }
int wf_jo_clock_advance(wf_jo_clock_t *c, uint32_t f) {
    uint32_t elapsed=f-c->last_field;
    uint64_t due;
    uint32_t remainder;
    if(elapsed<=25u && c->remainder<39168u) {
        /* 37609 = 39168 - 1559. Across at most25 fields the deficit is
         * smaller than one denominator, so one correction is exact. This
         * avoids software64-bit division on the normal SH-2 field path. */
        int32_t residual=(int32_t)c->remainder-(int32_t)(elapsed*1559u);
        due=elapsed;
        if(residual<0){--due;residual+=39168;}
        remainder=(uint32_t)residual;
    } else {
        uint64_t scaled=(uint64_t)elapsed*37609u+c->remainder;
        due=scaled/39168u;
        remainder=(uint32_t)(scaled%39168u);
    }
    if (due>UINT32_MAX-c->pending) return 0;
    c->last_field=f;
    c->remainder=remainder;
    c->pending+=(uint32_t)due;
    if(c->pending>c->max_pending)c->max_pending=c->pending;
    return 1;
}
int wf_jo_clock_take(wf_jo_clock_t *c) {
    if (!c->pending) return 0;
    --c->pending;
    return 1;
}
