#ifndef WP01_FM_H
#define WP01_FM_H
#include <stdint.h>

/* Host compiler / native bridge: callback receives byte offsets from SCSP base.
 * Native integration can precompile these writes and replay at event timestamps.
 * FM slots0..23 are mapped to original channels0,1,2,3,4,7; 24..31 remain free.
 * Copy FM-SINE.PCM to SCSP RAM0 before calling init. No song PCM is involved.
 */
typedef void (*wp01_fm_writer_t)(void *context, uint16_t offset, uint16_t value);
void wp01_fm_init(wp01_fm_writer_t writer, void *context);
void wp01_fm_load_registers(const uint8_t original_registers[256]);
void wp01_fm_write(uint8_t address, uint8_t value);
void wp01_fm_stop(void);
uint32_t wp01_fm_unsupported(void);
uint16_t wp01_fm_pitch_word(uint8_t kc, uint8_t kf, uint8_t dt_mul, uint8_t dt2);

enum { WP01_FM_SINE_BYTES = 266240, WP01_FM_SINE_SA = 65536,
       WP01_FM_CARRIER_SA = 0x30800 };
enum { WP01_FM_UNSUPPORTED_LFO=1, WP01_FM_UNSUPPORTED_NOISE=2,
       WP01_FM_UNSUPPORTED_ACTIVE_CHANNEL=4, WP01_FM_PITCH_CLAMPED=8 };
#endif
