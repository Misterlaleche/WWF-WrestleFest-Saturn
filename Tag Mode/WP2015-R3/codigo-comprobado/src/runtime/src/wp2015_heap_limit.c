/* Local WP2015 allocation boundary; the supplied Yaul SDK stays unchanged.
 * Link SH-2 with --wrap=tlsf_pool_create and wp2015_memory_asserts.ld.
 * This uses only the public TLSF API and the SDK's existing __end symbol. */
#include <stdint.h>
#include <stddef.h>
#include <mm/tlsf.h>

extern char __end;

#if defined(__sh__)
#define WP2015_WRAP_NAME "___wrap_tlsf_pool_create"
#define WP2015_REAL_NAME "___real_tlsf_pool_create"
#else
/* The independent host boundary test uses normal ELF C symbol names. */
#define WP2015_WRAP_NAME "__wrap_tlsf_pool_create"
#define WP2015_REAL_NAME "__real_tlsf_pool_create"
#endif

extern tlsf_t wp2015_real_pool_create(void *, size_t) __asm__(WP2015_REAL_NAME);
tlsf_t wp2015_limited_pool_create(void *, size_t) __asm__(WP2015_WRAP_NAME);

tlsf_t wp2015_limited_pool_create(void *memory, size_t bytes)
{
    const uintptr_t start = (uintptr_t)memory;
    if (start == (uintptr_t)&__end) {
        const uintptr_t limit = 0x060da000u;
        if (start >= limit)
            return NULL;
        const size_t available = (size_t)(limit - start);
        if (bytes > available)
            bytes = available;
        /* The final-link assertion guarantees this for the SDK user pool.
         * Retain the runtime guard so an invalid later request cannot write
         * the TLSF control block outside the caller's supplied region. */
        if (bytes < tlsf_size() + tlsf_pool_overhead() + tlsf_block_size_min())
            return NULL;
    }
    return wp2015_real_pool_create(memory, bytes);
}
