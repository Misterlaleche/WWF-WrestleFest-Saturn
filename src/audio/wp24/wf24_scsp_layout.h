#ifndef WF24_SCSP_LAYOUT_H
#define WF24_SCSP_LAYOUT_H

#include <stdbool.h>
#include <stdint.h>

enum {
    WF24_SCSP_RAM_BYTES = 0x80000u,
    WF24_SFX_RESIDENT_START = 0x14000u,
    WF24_SFX_RESIDENT_END = 0x3C5E4u,
    WF24_SFX_RESIDENT_ALIGNED_END = 0x3C600u,
    WF24_SFX_DYNAMIC_START = WF24_SFX_RESIDENT_ALIGNED_END,
    WF24_SFX_DYNAMIC_STRIDE = 0xA000u,
    WF24_SFX_DYNAMIC_SLOTS = 3u,
    WF24_SFX_DYNAMIC_END = WF24_SFX_DYNAMIC_START +
                           WF24_SFX_DYNAMIC_STRIDE * WF24_SFX_DYNAMIC_SLOTS,
    WF24_MUSIC_RING_START = 0x60000u,
    WF24_MUSIC_RING_BYTES_LAYOUT = 0x10000u,
    WF24_MUSIC_RING_END = WF24_MUSIC_RING_START + WF24_MUSIC_RING_BYTES_LAYOUT,
    WF24_SFX_MUSIC_GUARD_BYTES = WF24_MUSIC_RING_START - WF24_SFX_DYNAMIC_END
};

_Static_assert(WF24_SFX_RESIDENT_END <= WF24_SFX_DYNAMIC_START,
               "resident SFX region overlaps dynamic cache");
_Static_assert(WF24_SFX_DYNAMIC_END <= WF24_MUSIC_RING_START,
               "dynamic SFX cache overlaps music ring");
_Static_assert(WF24_MUSIC_RING_END <= WF24_SCSP_RAM_BYTES,
               "music ring exceeds SCSP RAM");
_Static_assert(WF24_MUSIC_RING_START == 0x60000u,
               "hardware-proven music ring must remain at 0x60000");
_Static_assert(WF24_SFX_DYNAMIC_STRIDE == 0xA000u,
               "A16 dynamic SFX slot size changed");
_Static_assert(WF24_SFX_DYNAMIC_SLOTS == 3u,
               "A16 logical SFX voice count changed");

bool wf24_scsp_span_valid(uint32_t start, uint32_t bytes);
bool wf24_scsp_layout_valid(void);

#endif
