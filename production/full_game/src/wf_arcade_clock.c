#include "wf_arcade_clock.h"
#include <stddef.h>
#include <string.h>

bool wf_arcade_clock_init(wf_arcade_clock_t *c,uint32_t phase) {
    if(c==NULL || phase>=WF_ARCADE_THRESHOLD) return false;
    memset(c,0,sizeof *c);
    c->phase=phase;
    return true;
}

bool wf_arcade_clock_field(wf_arcade_clock_t *c) {
    if(c==NULL || c->failed) return false;
    if(c->phase>=WF_ARCADE_THRESHOLD || c->fields==UINT64_MAX) {
        c->failed=true;
        return false;
    }
    uint32_t phase=c->phase+WF_ARCADE_ADD;
    if(phase>=WF_ARCADE_THRESHOLD && c->pending==UINT32_MAX) {
        c->failed=true;
        return false;
    }
    ++c->fields;
    if(phase>=WF_ARCADE_THRESHOLD) {
        phase-=WF_ARCADE_THRESHOLD;
        ++c->pending;
    }
    c->phase=phase;
    return true;
}

bool wf_arcade_clock_take(wf_arcade_clock_t *c) {
    if(c==NULL || c->failed || c->pending==0) return false;
    if(c->ticks==UINT64_MAX) {
        c->failed=true;
        return false;
    }
    --c->pending;
    ++c->ticks;
    return true;
}
