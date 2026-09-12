#include <assert.h>
#include <stdint.h>
#include "wf24_scsp_layout.h"
static uint32_t align32(uint32_t n) { return (n + 31u) & ~31u; }
int main(void)
{
    static const uint32_t resident_bytes[] = {
        1802u,3700u,762u,6932u,7998u,3844u,5128u,9398u,12998u,
        1898u,5178u,8198u,4718u,4458u,12058u,1154u,31978u,32018u,
        3004u,2714u,5092u
    };
    uint32_t addr = WF24_SFX_RESIDENT_START;
    unsigned i;
    for (i = 0u; i < sizeof(resident_bytes)/sizeof(resident_bytes[0]); ++i) {
        addr = align32(addr);
        addr += resident_bytes[i];
    }
    assert(addr == WF24_SFX_RESIDENT_END);
    assert(align32(addr) == WF24_SFX_DYNAMIC_START);
    assert(WF24_SFX_DYNAMIC_END == 0x5A600u);
    assert(WF24_SFX_MUSIC_GUARD_BYTES == 0x5A00u);
    assert(WF24_MUSIC_RING_START == 0x60000u);
    assert(WF24_MUSIC_RING_END == 0x70000u);
    assert(wf24_scsp_layout_valid());
    return 0;
}
