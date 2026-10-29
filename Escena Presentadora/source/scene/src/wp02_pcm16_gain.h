#ifndef WP02_PCM16_GAIN_H
#define WP02_PCM16_GAIN_H
#include <stdbool.h>
#include <stdint.h>

/* The pinned 8732-byte driver uses a 3-bit direct-send level, not TL.
 * Apply the original OKI multiplier to the original PCM at preload, then
 * send at native level7. The raw ROM-derived pack stays unchanged.
 * Fractional PCM16 LSBs round to nearest/even; no resampling is performed. */
static inline bool wp02_apply_oki_gain_be(uint8_t *pcm, unsigned bytes,
                                         unsigned volume_nibble)
{
    static const uint8_t numerator[16] = {
        32,22,16,11,8,6,4,3,2,0,0,0,0,0,0,0
    };
    if (!pcm || (bytes & 1u) || volume_nibble >= 16u) return false;
    for (unsigned i = 0; i < bytes; i += 2u) {
        int32_t value = (int16_t)(((uint16_t)pcm[i] << 8) | pcm[i + 1u]);
        int32_t product = value * numerator[volume_nibble];
        int32_t scaled = product / 32;
        unsigned magnitude = (unsigned)(product < 0 ? -product : product);
        unsigned remainder = magnitude & 31u;
        if (remainder > 16u || (remainder == 16u && (scaled & 1)))
            scaled += product < 0 ? -1 : 1;
        uint16_t result = (uint16_t)(int16_t)scaled;
        pcm[i] = (uint8_t)(result >> 8);
        pcm[i + 1u] = (uint8_t)result;
    }
    return true;
}
#endif
