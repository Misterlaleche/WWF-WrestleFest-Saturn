#include "wf24_scsp_layout.h"

bool
wf24_scsp_span_valid(uint32_t start, uint32_t bytes)
{
    if (start > WF24_SCSP_RAM_BYTES)
        return false;
    return bytes <= WF24_SCSP_RAM_BYTES - start;
}

bool
wf24_scsp_layout_valid(void)
{
    return WF24_SFX_RESIDENT_START < WF24_SFX_RESIDENT_END &&
           WF24_SFX_RESIDENT_END <= WF24_SFX_DYNAMIC_START &&
           WF24_SFX_DYNAMIC_START < WF24_SFX_DYNAMIC_END &&
           WF24_SFX_DYNAMIC_END <= WF24_MUSIC_RING_START &&
           WF24_MUSIC_RING_START < WF24_MUSIC_RING_END &&
           WF24_MUSIC_RING_END <= WF24_SCSP_RAM_BYTES &&
           wf24_scsp_span_valid(WF24_SFX_RESIDENT_START,
                                WF24_SFX_RESIDENT_END - WF24_SFX_RESIDENT_START) &&
           wf24_scsp_span_valid(WF24_SFX_DYNAMIC_START,
                                WF24_SFX_DYNAMIC_END - WF24_SFX_DYNAMIC_START) &&
           wf24_scsp_span_valid(WF24_MUSIC_RING_START,
                                WF24_MUSIC_RING_BYTES_LAYOUT);
}
