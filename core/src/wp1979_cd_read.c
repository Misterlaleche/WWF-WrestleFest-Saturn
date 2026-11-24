/* Native Saturn CD sector transport, using the 32-bit A-bus FIFO alias.
 *
 * The supplied libyaul reads 16-bit words from 0x25890000. Yabause's sector
 * FIFO instead implements long reads at 0x25818000, matching the retail BIOS
 * sector transfer path. This is ordinary Saturn bus I/O, not an emulator API.
 * See README.md for Sega's access-method bulletin and primary bus evidence.
 */
#include "wp1979_cd_read.h"
#include <cpu/intc.h>
#include <stddef.h>
#include <stdint.h>

#define REG16(address) (*(volatile uint16_t *)(uintptr_t)(address))
#define FIFO32 (*(volatile uint32_t *)(uintptr_t)0x25818000u)
#define HIRQ REG16(0x25890008u)
#define CR1 REG16(0x25890018u)
#define CR2 REG16(0x2589001cu)
#define CR3 REG16(0x25890020u)
#define CR4 REG16(0x25890024u)
#define CMOK 0x0001u
#define DRDY 0x0002u
#define CSCT 0x0004u
#define PEND 0x0010u
#define ESEL 0x0040u
#define EHST 0x0080u
#define EFLS 0x0200u
#define POLL_LIMIT 0x240000u
#define EMPTY_COMMAND_LIMIT 16384u
#define SECTOR_BYTES 2048u

volatile wp1979_cd_diagnostics_t wp1979_cd_diagnostics;

typedef struct { uint16_t cr1, cr2, cr3, cr4; } response_t;

static int wait_flag(uint16_t flag)
{
    for (uint32_t i = 0; i < POLL_LIMIT; ++i)
        if ((HIRQ & flag) != 0u) return 0;
    return -1;
}

static int command(uint16_t cr1, uint16_t cr2, uint16_t cr3,
                   uint16_t cr4, uint16_t clear, response_t *response)
{
    wp1979_cd_diagnostics.command[0] = cr1;
    wp1979_cd_diagnostics.command[1] = cr2;
    wp1979_cd_diagnostics.command[2] = cr3;
    wp1979_cd_diagnostics.command[3] = cr4;
    uint8_t mask = cpu_intc_mask_get();
    cpu_intc_mask_set(15);
    if (wait_flag(CMOK)) {
        cpu_intc_mask_set(mask);
        return WP1979_CD_COMMAND_TIMEOUT;
    }
    /* A zero acknowledges a flag; ones preserve unrelated pending flags. */
    HIRQ = (uint16_t)~(CMOK | clear);
    CR1 = cr1; CR2 = cr2; CR3 = cr3; CR4 = cr4;
    if (wait_flag(CMOK)) {
        cpu_intc_mask_set(mask);
        return WP1979_CD_COMMAND_TIMEOUT;
    }
    response->cr1 = CR1;
    response->cr2 = CR2;
    response->cr3 = CR3;
    response->cr4 = CR4;
    cpu_intc_mask_set(mask);
    wp1979_cd_diagnostics.response[0] = response->cr1;
    wp1979_cd_diagnostics.response[1] = response->cr2;
    wp1979_cd_diagnostics.response[2] = response->cr3;
    wp1979_cd_diagnostics.response[3] = response->cr4;
    /* CR1 high byte is status; low byte is flags/repeat or command data.
     * Drive states are ordinal values, not independent bit flags. */
    uint8_t status = (uint8_t)(response->cr1 >> 8);
    if (status == 0xffu) return WP1979_CD_REJECT;
    if (status & 0x80u) return WP1979_CD_WAIT;
    switch (status & 0x0fu) {
        case 6: return WP1979_CD_OPEN;
        case 7: return WP1979_CD_NO_DISC;
        case 9: return WP1979_CD_ERROR;
        case 10: return WP1979_CD_FATAL;
        default: return 0;
    }
}

static int setting(uint16_t cr1, uint16_t cr2, uint16_t cr3,
                   uint16_t clear, response_t *response)
{
    int result = command(cr1, cr2, cr3, 0, clear, response);
    if (result) return result;
    return wait_flag(ESEL) ? WP1979_CD_READY_TIMEOUT : 0;
}

int wp1979_cd_sectors_read(fad_t fad, void *output_buffer, uint32_t length)
{
    response_t response;
    uint8_t *out = output_buffer;
    uint32_t sectors = length / SECTOR_BYTES + (length % SECTOR_BYTES != 0u);
    int result = 0, transferring = 0;
    wp1979_cd_diagnostics.stage = 0;
    wp1979_cd_diagnostics.fad = (uint32_t)fad;
    wp1979_cd_diagnostics.requested_bytes = length;
    wp1979_cd_diagnostics.copied_bytes = 0;
    wp1979_cd_diagnostics.error = 0;
    if (!out || !length || fad < 150 || (uint32_t)fad > 0x7fffffu ||
        sectors > 0xffffu || sectors > 0x800000u - (uint32_t)fad ||
        (uintptr_t)out > UINTPTR_MAX - (length - 1u)) {
        wp1979_cd_diagnostics.error = WP1979_CD_ARGUMENT;
        return WP1979_CD_ARGUMENT;
    }

    wp1979_cd_diagnostics.stage = 1;
    if ((result = setting(0x6000u, 0, 0, ESEL, &response))) goto failed;
    wp1979_cd_diagnostics.stage = 2;
    if ((result = setting(0x4800u, 0, 0, ESEL, &response))) goto failed;
    wp1979_cd_diagnostics.stage = 3;
    if ((result = setting(0x3000u, 0, 0, ESEL, &response))) goto failed;
    wp1979_cd_diagnostics.stage = 4;
    if ((result = command((uint16_t)(0x1080u | ((uint32_t)fad >> 16)),
                          (uint16_t)fad, 0x0080u, (uint16_t)sectors,
                          PEND | CSCT, &response))) goto failed;

    uint32_t bytes_left = length;
    while (sectors) {
        uint32_t available = 0;
        wp1979_cd_diagnostics.stage = 5;
        for (uint32_t poll = 0; poll < EMPTY_COMMAND_LIMIT; ++poll) {
            if ((result = command(0x5100u, 0, 0, 0, 0, &response))) goto failed;
            available = response.cr4;
            if (available) break;
        }
        if (!available) { result = WP1979_CD_SECTOR_TIMEOUT; goto failed; }
        if (available > 200u) { result = WP1979_CD_BUFFER_COUNT; goto failed; }
        uint32_t take = available < sectors ? available : sectors;
        uint32_t transfer_bytes = take * SECTOR_BYTES;
        wp1979_cd_diagnostics.stage = 6;
        if ((result = command(0x6300u, 0, 0, (uint16_t)take,
                              DRDY | EHST, &response))) goto failed;
        transferring = 1;
        wp1979_cd_diagnostics.stage = 7;
        if (wait_flag(DRDY)) { result = WP1979_CD_READY_TIMEOUT; goto failed; }
        HIRQ = (uint16_t)~DRDY;
        wp1979_cd_diagnostics.stage = 8;
        if (((uintptr_t)out & 3u) == 0u && bytes_left >= transfer_bytes) {
            uint32_t *words = (uint32_t *)(void *)out;
            for (uint32_t i = 0; i < transfer_bytes / 4u; ++i) words[i] = FIFO32;
            out += transfer_bytes;
            bytes_left -= transfer_bytes;
        } else {
            /* Consume complete sectors even for an unaligned/partial final
             * request, while writing exactly the caller's byte capacity. */
            for (uint32_t i = 0; i < transfer_bytes / 4u; ++i) {
                uint32_t word = FIFO32;
                for (unsigned b = 0; b < 4u; ++b) {
                    if (bytes_left) { *out++ = (uint8_t)(word >> 24); --bytes_left; }
                    word <<= 8;
                }
            }
        }
        wp1979_cd_diagnostics.copied_bytes = length - bytes_left;
        wp1979_cd_diagnostics.stage = 9;
        if ((result = command(0x0600u, 0, 0, 0, 0, &response))) goto failed;
        transferring = 0;
        uint32_t transferred_words = ((uint32_t)(response.cr1 & 255u) << 16) | response.cr2;
        if (transferred_words != transfer_bytes / 2u) {
            result = WP1979_CD_TRANSFER_COUNT;
            goto failed;
        }
        sectors -= take;
    }
    wp1979_cd_diagnostics.stage = 10;
    return 0;

failed:
    /* Best-effort cleanup leaves retry/setup possible; preserve the original
     * failure and diagnostic response, including if cleanup also times out. */
    {
        wp1979_cd_diagnostics_t saved = wp1979_cd_diagnostics;
        if (transferring) (void)command(0x0600u, 0, 0, 0, 0, &response);
        (void)command(0x7500u, 0, 0, 0, EFLS, &response);
        wp1979_cd_diagnostics = saved;
        wp1979_cd_diagnostics.error = result;
    }
    return result;
}

int wp1979_cd_sector_read(fad_t fad, void *output_buffer)
{
    return wp1979_cd_sectors_read(fad, output_buffer, SECTOR_BYTES);
}
