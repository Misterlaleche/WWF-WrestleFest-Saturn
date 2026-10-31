#ifndef WF_ARCADE_CLOCK_H
#define WF_ARCADE_CLOCK_H
#include <stdbool.h>
#include <stdint.h>

/* Ratio from approved Entrada Ring/source/main.c. Physical fields are not
 * gameplay ticks. Call from the master owner, not concurrently from an ISR.
 * The presentation/ACK owner consumes ticks; loading does not advance this
 * clock. Phase is supplied explicitly at the proven scene/time boundary. */
enum { WF_ARCADE_ADD=37609, WF_ARCADE_THRESHOLD=39168 };
typedef struct {
    uint32_t phase, pending;
    uint64_t fields, ticks;
    bool failed;
} wf_arcade_clock_t;
bool wf_arcade_clock_init(wf_arcade_clock_t *clock,uint32_t phase);
bool wf_arcade_clock_field(wf_arcade_clock_t *clock);
bool wf_arcade_clock_take(wf_arcade_clock_t *clock);
#endif
