#ifndef WP1979_CD_READ_H
#define WP1979_CD_READ_H

#include <cd-block.h>
#include <stdint.h>

enum {
    WP1979_CD_ARGUMENT = -1,
    WP1979_CD_COMMAND_TIMEOUT = -2,
    WP1979_CD_READY_TIMEOUT = -3,
    WP1979_CD_SECTOR_TIMEOUT = -4,
    WP1979_CD_REJECT = -5,
    WP1979_CD_WAIT = -6,
    WP1979_CD_OPEN = -7,
    WP1979_CD_NO_DISC = -8,
    WP1979_CD_ERROR = -9,
    WP1979_CD_FATAL = -10,
    WP1979_CD_BUFFER_COUNT = -11,
    WP1979_CD_TRANSFER_COUNT = -12
};

/* Synchronous, single-owner CD transport. The CD block must be initialized.
 * FAD is LBA + 150. The caller supplies a CPU-writable destination of at
 * least length bytes. Any byte alignment is supported. The source command
 * partition is selector 0, matching the existing R18 transport.
 *
 * Returns 0 on success or a negative value above. Polling is bounded without
 * changing FRT configuration, which the sound boot path also uses.
 */
int wp1979_cd_sector_read(fad_t fad, void *output_buffer);
int wp1979_cd_sectors_read(fad_t fad, void *output_buffer, uint32_t length);

typedef struct {
    uint32_t stage, fad, requested_bytes, copied_bytes;
    uint16_t command[4], response[4];
    int32_t error;
} wp1979_cd_diagnostics_t;
extern volatile wp1979_cd_diagnostics_t wp1979_cd_diagnostics;

#endif
