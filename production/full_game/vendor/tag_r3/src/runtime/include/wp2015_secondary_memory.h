#ifndef WP2015_SECONDARY_MEMORY_H
#define WP2015_SECONDARY_MEMORY_H
#include <stdint.h>
#include "wf_saturn_memory.h"
#include "wp2015_journal.h"

/* CPU-owned immutable bytes only. No framebuffer, SDK pool, heap, palette,
 * command, live texture, font, name, ring-character or back-color bytes. */
#define WP2015_SECONDARY_VDP1_BYTES 162208u
#define WP2015_SECONDARY_VDP2_BYTES 150752u
#define WP2015_SECONDARY_VRAM_BYTES (WP2015_SECONDARY_VDP1_BYTES+WP2015_SECONDARY_VDP2_BYTES)
#define WP2015_SECONDARY_RING_BYTES 0x19180u
#define WP2015_SECONDARY_JOURNAL_END 0x69320u
_Static_assert(WP2015_JOURNAL_ADDRESS-0x05c00000u+WP2015_JOURNAL_BYTES==WP2015_SECONDARY_JOURNAL_END,"Compact journal boundary");
_Static_assert(WF_SATURN_VDP1_CROP_END<=0x452a0u,"Crop table boundary");
_Static_assert(WF_SATURN_VDP1_DYNAMIC_OFFSET==0x4b580u&&WF_SATURN_VDP1_DYNAMIC_END==0x5b580u,"Both texture banks remain owned");
_Static_assert(WF_SATURN_VDP1_JOURNAL_OFFSET==0x5c000u,"Journal starts after texture gap");
_Static_assert(WF_SATURN_VDP2_BACK_COLOR>=0x1ffe0u,"Back-color guard");

typedef struct {uint32_t address,bytes;} wp2015_secondary_region_t;
static const wp2015_secondary_region_t wp2015_secondary_vram[] = {
    {0x25c360a0u,0x40000u-0x360a0u},
    {0x25c452a0u,0x4b580u-0x452a0u},
    {0x25c5b580u,0x5c000u-0x5b580u},
    {0x25c69320u,0x80000u-0x69320u},
    {0x25e19180u,0x20000u-0x19180u},
    {0x25e59180u,0x20000u-0x19180u},
    {0x25e20800u,0x1800u},{0x25e22800u,0x1800u},
    {0x25e26800u,0x1800u},{0x25e28800u,0x1800u},
    {0x25e2c800u,0x1800u},{0x25e2e800u,0x1800u},
    {0x25e32000u,0x1ffe0u-0x12000u}
};
#define WP2015_SECONDARY_REGIONS (sizeof wp2015_secondary_vram/sizeof wp2015_secondary_vram[0])
#endif
