#ifndef WF_ROSTER_IDENTITY_H
#define WF_ROSTER_IDENTITY_H
#include <stdbool.h>
#include <stdint.h>
/* WORD table $64CC, as consumed by $5DA0. Only identity conversion, not team
 * canonicalization, role flags, match setup or RNG. Failure leaves output intact. */
bool wf_roster_raw_identity(uint16_t slot,uint8_t *raw);
#endif
