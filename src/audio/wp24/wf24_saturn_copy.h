#ifndef WF24_SATURN_COPY_H
#define WF24_SATURN_COPY_H

#include <stdbool.h>
#include <stdint.h>

/* Saturn-only data movement. No slot control and no KYONEX access lives here. */
bool wf24_saturn_copy_half_be(unsigned half, const void *src_be,
                              uint32_t bytes, void *user);

#endif
