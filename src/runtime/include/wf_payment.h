#ifndef WF_PAYMENT_H
#define WF_PAYMENT_H
#include <stdint.h>
#include "wf_runtime.h"

/* Exact $00055A payment consumer. Return 1 iff ROM carry would be set. */
int wf_payment_consume_055a(wf_runtime_t *rt,uint8_t mode);
#endif
