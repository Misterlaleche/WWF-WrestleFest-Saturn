#ifndef WF_NATIVE_ELF_H
#define WF_NATIVE_ELF_H
#include "wf_native_host.h"
/* Borrow the complete preloaded ELF through host_boot only. No allocations,
 * no file reads and no bus writes. Validate before SNDOFF. */
bool wf_native_elf_image(const void *,size_t,wf_native_segment_t segments[3],
                         wf_native_image_t *);
#endif
