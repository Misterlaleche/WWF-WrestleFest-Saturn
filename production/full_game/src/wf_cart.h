#ifndef WF_CART_H
#define WF_CART_H
#include <stdbool.h>
#include <stdint.h>
/* Boot-only test, before any cart allocation. SDK detection runs first.
 * Bus operations must be uncached volatile accesses on Saturn. */
typedef struct {
 uint32_t (*read)(void *,uint32_t);
 void (*write)(void *,uint32_t,uint32_t);
 void *context;
} wf_cart_bus_t;
bool wf_cart_validate(uint32_t sdk_id,uint32_t sdk_bytes,uint32_t sdk_base,
                      const wf_cart_bus_t *bus);
#endif
