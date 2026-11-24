#ifndef WF_JO_CLOCK_H
#define WF_JO_CLOCK_H
#include <stdint.h>
typedef struct {
    uint32_t last_field, remainder, pending, max_pending;
} wf_jo_clock_t;
void wf_jo_clock_start(wf_jo_clock_t *, uint32_t field);
int wf_jo_clock_advance(wf_jo_clock_t *, uint32_t field);
int wf_jo_clock_take(wf_jo_clock_t *);
#endif
