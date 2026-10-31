#ifndef WP2015_CD_RATE_H
#define WP2015_CD_RATE_H
#include <stdint.h>

enum {
    WP2015_CD_RATE_NOT_BOUND = -810,
    WP2015_CD_RATE_CLOCK = -811,
    WP2015_CD_RATE_TIMER_TIMEOUT = -812
};

/* One master-CPU CD owner, shared by CDFS, retained reads, writes and cleanup.
 * Bind before the first WP2015 transport command. It separates prior SDK
 * commands by one full long window. The counter must advance once per VBlank.
 * No API changes FRT or the interrupt mask. Native FRT /8, /32 and /128 are
 * accepted; external-clock mode is rejected. */
int wp2015_cd_rate_bind(volatile uint32_t *video_clock);
/* Call before masking IRQs or waiting for CMOK. A successful admission remains
 * valid while time passes, provided no other owner issues a command. */
int wp2015_cd_rate_wait(void);
/* Call immediately AFTER CR4, before waiting for the response. This timestamp
 * bounds the actual emission from above. It must also run if response times out. */
void wp2015_cd_rate_issued(void);
/* Call immediately before every master FRT reset/divisor change. Drains the
 * previous clock epoch; no CD command may intervene before the actual reset. */
int wp2015_cd_rate_before_frt_reset(void);

#endif
