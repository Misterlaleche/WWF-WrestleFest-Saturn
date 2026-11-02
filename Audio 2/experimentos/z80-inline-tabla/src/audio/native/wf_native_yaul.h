#ifndef WF_NATIVE_YAUL_H
#define WF_NATIVE_YAUL_H
#include "wf_native_host.h"
/* Supply the game's monotonic timer, wait/idle and owner callbacks first.
 * This fills SMPC, uncached 16-bit bus writes and the bus ordering callback.
 * Call wf_native_host_init with mailbox at 0x25a0c040.
 * Select this boot path instead of wf_ponesound_load_driver, never both. */
void wf_native_yaul_complete_platform(wf_native_platform_t *ops);
#endif
