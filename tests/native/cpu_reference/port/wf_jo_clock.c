#include "wf_jo_clock.h"
void wf_jo_clock_start(wf_jo_clock_t *c, uint32_t f) { *c=(wf_jo_clock_t){f,0,0,0}; }
int wf_jo_clock_advance(wf_jo_clock_t *c, uint32_t f) {
    uint32_t elapsed=f-c->last_field;
    uint64_t scaled=(uint64_t)elapsed*37609u+c->remainder;
    uint64_t due=scaled/39168u;
    if (due>UINT32_MAX-c->pending) return 0;
    c->last_field=f;
    c->remainder=(uint32_t)(scaled%39168u);
    c->pending+=(uint32_t)due;
    if(c->pending>c->max_pending)c->max_pending=c->pending;
    return 1;
}
int wf_jo_clock_take(wf_jo_clock_t *c) {
    if (!c->pending) return 0;
    --c->pending;
    return 1;
}
