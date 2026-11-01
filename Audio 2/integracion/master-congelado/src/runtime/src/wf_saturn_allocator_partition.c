/* The SDK passes its 40 KiB private storage to tlsf_pool_create. Reserve the
 * middle 18 KiB for music and final 8 KiB for the slave stack before TLSF
 * sees that storage. The first 14 KiB belongs to private TLSF. User malloc's
 * separate pool still starts at ___end and extends to HWRAM's exclusive end. */
#include <mm/tlsf.h>
#include <mm/mm_stats.h>
#include <stdint.h>

extern unsigned char __wf_sdk_private_pool_start[];
extern unsigned char __end[];
extern void *__malloc(size_t);
extern void __free(void *);
#if defined(__GNUC__) && defined(__sh__)
extern tlsf_t wf_saturn_real_pool_create(void *, size_t)
    __asm__("___real_tlsf_pool_create");
extern tlsf_pool_t wf_saturn_real_pool_get(tlsf_t)
    __asm__("___real_tlsf_pool_get");
#else
extern tlsf_t wf_saturn_real_pool_create(void *, size_t);
extern tlsf_pool_t wf_saturn_real_pool_get(tlsf_t);
#endif

/* Readable in a debugger without allocating or walking the running pool.
 * Fields are raw private bytes, useful private bytes, raw user bytes,
 * useful user bytes, TLSF control bytes, TLSF pool overhead, private handle,
 * user handle, user control address, its allocated block bytes, allocation
 * overhead, actual user pool address. SDK mm_stats_walk remains valid. */
volatile uint32_t wf_saturn_allocator_partition[12];
static tlsf_t user_handle;
static tlsf_pool_t user_pool;

#if defined(__GNUC__)
__attribute__((used, externally_visible))
#endif
tlsf_t wf_saturn_tlsf_pool_create(void *base, size_t bytes)
{
    tlsf_t pool;
    size_t overhead = tlsf_size() + tlsf_pool_overhead();
    if (base == (void *)__wf_sdk_private_pool_start) {
        if (bytes != 0xa000u) return NULL;
        bytes = 0x3800u;
        wf_saturn_allocator_partition[0] = (uint32_t)bytes;
        wf_saturn_allocator_partition[1] = (uint32_t)(bytes - overhead);
    } else if (base == (void *)__end) {
        void *control;
        wf_saturn_allocator_partition[2] = (uint32_t)bytes;
        wf_saturn_allocator_partition[3] = 0;
        wf_saturn_allocator_partition[7] = 0;
        wf_saturn_allocator_partition[8] = 0;
        wf_saturn_allocator_partition[9] = 0;
        wf_saturn_allocator_partition[10] = 0;
        wf_saturn_allocator_partition[11] = 0;
        user_handle = NULL;
        user_pool = NULL;
        if (bytes <= tlsf_pool_overhead()) return NULL;
        /* __mm_init has installed the first handle in its JSR delay slot.
         * Allocate only the control structure from that already valid pool;
         * every byte of the user arena remains outside the private/stack area. */
        control = __malloc(tlsf_size());
        if (control == NULL) return NULL;
        pool = tlsf_create(control);
        if (pool == NULL) { __free(control); return NULL; }
        user_pool = tlsf_pool_add(pool, base, bytes);
        if (user_pool == NULL) { __free(control); return NULL; }
        user_handle = pool;
        wf_saturn_allocator_partition[3] = (uint32_t)(bytes - tlsf_pool_overhead());
        wf_saturn_allocator_partition[7] = (uint32_t)(uintptr_t)pool;
        wf_saturn_allocator_partition[8] = (uint32_t)(uintptr_t)control;
        wf_saturn_allocator_partition[9] = (uint32_t)tlsf_block_size(control);
        wf_saturn_allocator_partition[10] = (uint32_t)tlsf_alloc_overhead();
        wf_saturn_allocator_partition[11] = (uint32_t)(uintptr_t)user_pool;
        return pool;
    }
    wf_saturn_allocator_partition[4] = (uint32_t)tlsf_size();
    wf_saturn_allocator_partition[5] = (uint32_t)tlsf_pool_overhead();
    pool = wf_saturn_real_pool_create(base, bytes);
    if (base == (void *)__wf_sdk_private_pool_start)
        wf_saturn_allocator_partition[6] = (uint32_t)(uintptr_t)pool;
    return pool;
}

/* Yaul's stats walker asks for the first pool using this API. An external
 * control structure changes that address; preserve the actual arena pointer. */
#if defined(__GNUC__)
__attribute__((used, externally_visible))
#endif
tlsf_pool_t wf_saturn_tlsf_pool_get(tlsf_t pool)
{
    if (user_handle != NULL && pool == user_handle) return user_pool;
    return wf_saturn_real_pool_get(pool);
}

/* Captures, faults; private used/free/largest-free/used-blocks/free-blocks;
 * user used/free/largest-free/used-blocks/free-blocks; SDK-reported private
 * and user pool sizes; private and user TLSF consistency results. */
volatile uint32_t wf_saturn_allocator_live_stats[16];

static void allocator_walk(const mm_stats_walk_entry_t *entry)
{
    uintptr_t base;
    size_t capacity;
    unsigned offset;
    if (entry->pool_type == MM_STATS_TYPE_YAUL) {
        base = (uintptr_t)__wf_sdk_private_pool_start;
        capacity = 14336;
        offset = 2;
        wf_saturn_allocator_live_stats[12] = (uint32_t)entry->pool_size;
    } else if (entry->pool_type == MM_STATS_TYPE_USER) {
        base = (uintptr_t)__end;
        capacity = wf_saturn_allocator_partition[2];
        offset = 7;
        wf_saturn_allocator_live_stats[13] = (uint32_t)entry->pool_size;
    } else {
        wf_saturn_allocator_live_stats[1] |= 128u;
        return;
    }
    if (entry->address < base || entry->size > capacity ||
        entry->address - base > capacity - entry->size) {
        wf_saturn_allocator_live_stats[1] |= entry->pool_type == MM_STATS_TYPE_YAUL ? 1u : 2u;
        return;
    }
    wf_saturn_allocator_live_stats[offset + (entry->used ? 0u : 1u)] += (uint32_t)entry->size;
    wf_saturn_allocator_live_stats[offset + (entry->used ? 3u : 4u)]++;
    if (!entry->used && entry->size > wf_saturn_allocator_live_stats[offset + 2])
        wf_saturn_allocator_live_stats[offset + 2] = (uint32_t)entry->size;
}

/* Call from the master at an explicit diagnostic checkpoint, never in VBlank
 * or per-frame gameplay. No allocation or output occurs during this walk. */
void wf_saturn_allocator_stats_capture(void)
{
    tlsf_t private_handle = (tlsf_t)(uintptr_t)wf_saturn_allocator_partition[6];
    uint32_t count = wf_saturn_allocator_live_stats[0] + 1u;
    unsigned i;
    for (i = 0; i < 16; ++i) wf_saturn_allocator_live_stats[i] = 0;
    wf_saturn_allocator_live_stats[0] = count;
    if (private_handle == NULL || user_handle == NULL) {
        wf_saturn_allocator_live_stats[1] = 64u;
        return;
    }
    wf_saturn_allocator_live_stats[14] = (uint32_t)(tlsf_check(private_handle) |
        tlsf_pool_check(tlsf_pool_get(private_handle)));
    wf_saturn_allocator_live_stats[15] = (uint32_t)(tlsf_check(user_handle) |
        tlsf_pool_check(tlsf_pool_get(user_handle)));
    if (wf_saturn_allocator_live_stats[14] || wf_saturn_allocator_live_stats[15])
        wf_saturn_allocator_live_stats[1] |= 32u;
    mm_stats_walk(allocator_walk, NULL);
}
