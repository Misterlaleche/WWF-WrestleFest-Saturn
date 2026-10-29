#ifndef WP02_PCM16_H
#define WP02_PCM16_H
#include <stdint.h>
/* Native poneSound type 0, big-endian signed 16-bit, centered, 8 kHz.
 * Only SH-2 owned descriptor fields are changed. Driver slot ownership stays
 * with poneSound/M68K. The caller issues the start separately. */
void wp02_pcm16_configure(unsigned logical_voice, unsigned sample_count,
                         uint32_t scsp_offset);
/* Semi-protected mode permits an intentional new cue to restart its slot.
 * The original OKI F1 rule is enforced by the scene adapter before calling. */
#define WP02_PCM_SEMI (-2)
#endif
