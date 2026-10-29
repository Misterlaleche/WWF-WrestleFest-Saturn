/*
 * WP0 local CD reader. Command/init sequencing is derived from libyaul.
 * Copyright (c) 2012-2023 Israel Jacquez <mrkotfw@gmail.com>
 * Original contributors: Israel Jacquez and Romulo Fernandes Machado.
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
 * LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 * OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * Sector data uses the 32-bit A-bus transfer area at physical 0x05818000
 * (uncached SH2 address 0x25818000). Commands remain 16-bit accesses in
 * the original library. This is an actual Saturn transfer address, not
 * an emulator-specific service. The existing SDK object also defines
 * init and the two sector readers; those are supplied here so that its
 * old 16-bit transfer object is not linked from libyaul.a.
 */
#include <yaul.h>
#include <stdint.h>

#define WP0_CD_HIRQ (*(volatile uint16_t *)0x25890008UL)
#define WP0_CD_DATA32 (*(volatile uint32_t *)0x25818000UL)
#define WP0_CD_CMOK 0x0001U
#define WP0_CD_DRDY 0x0002U
#define WP0_CD_ESEL 0x0040U
#define WP0_CD_TIMEOUT 0xAA

static int
wp0_cd_wait(uint16_t flag)
{
    for (volatile uint32_t i = 0; i < 0x240000U; ++i) {
        if ((WP0_CD_HIRQ & flag) != 0) return 0;
    }
    return WP0_CD_TIMEOUT;
}

int
cd_block_init(void)
{
    int rc;
    if ((rc = wp0_cd_wait(WP0_CD_CMOK)) != 0) return rc;
    if ((rc = cd_block_cmd_file_abort()) != 0) return rc;
    if ((rc = cd_block_cmd_cd_system_init(0x0002)) != 0) return rc;
    if ((rc = cd_block_cmd_data_transfer_end()) != 0) return rc;
    if ((rc = cd_block_cmd_selector_reset(0xFC, 0)) != 0) return rc;
    return wp0_cd_wait(WP0_CD_ESEL);
}

int
cd_block_transfer_data(uint16_t offset, uint16_t buffer_number,
                      uint8_t *output_buffer, uint32_t buffer_length)
{
    assert(output_buffer != NULL);
    assert(buffer_length > 0);
    const uint32_t count = (buffer_length + CDFS_SECTOR_SIZE - 1) /
                           CDFS_SECTOR_SIZE;
    int rc = cd_block_cmd_sector_data_get_delete(offset, buffer_number, count);
    if (rc != 0) return rc;
    /* DRDY means that data is ready. EHST can still describe a prior transfer. */
    if ((rc = wp0_cd_wait(WP0_CD_DRDY)) != 0) return rc;

    uint32_t remaining = buffer_length;
    if (((uintptr_t)output_buffer & 3U) == 0) {
        uint32_t *words = (uint32_t *)output_buffer;
        while (remaining >= 4) {
            *words++ = WP0_CD_DATA32;
            remaining -= 4;
        }
        output_buffer = (uint8_t *)words;
    }
    while (remaining != 0) {
        uint32_t word = WP0_CD_DATA32;
        const uint32_t bytes = remaining < 4 ? remaining : 4;
        for (uint32_t i = 0; i < bytes; ++i) {
            *output_buffer++ = (uint8_t)(word >> 24);
            word <<= 8;
        }
        remaining -= bytes;
    }
    return cd_block_cmd_data_transfer_end();
}

int
cd_block_sectors_read(fad_t fad, void *output_buffer, uint32_t length)
{
    assert(fad >= 150);
    assert(output_buffer != NULL);
    assert(length > 0);
    const uint32_t count = (length + CDFS_SECTOR_SIZE - 1) / CDFS_SECTOR_SIZE;
    int rc;
    if ((rc = cd_block_cmd_sector_length_set(SECTOR_LENGTH_2048)) != 0) return rc;
    if ((rc = cd_block_cmd_selector_reset(0, 0)) != 0) return rc;
    if ((rc = cd_block_cmd_cd_dev_connection_set(0)) != 0) return rc;
    if ((rc = cd_block_cmd_disk_play(0, fad, count)) != 0) return rc;

    uint8_t *dest = output_buffer;
    uint32_t remaining = length;
    while (remaining != 0) {
        int ready;
        do {
            ready = cd_block_cmd_sector_number_get(0);
            if (ready < 0) return ready;
        } while (ready == 0);
        uint32_t bytes = (uint32_t)ready * CDFS_SECTOR_SIZE;
        if (bytes > remaining) bytes = remaining;
        if ((rc = cd_block_transfer_data(0, 0, dest, bytes)) != 0) return rc;
        dest += bytes;
        remaining -= bytes;
    }
    return 0;
}

int
cd_block_sector_read(fad_t fad, void *output_buffer)
{
    return cd_block_sectors_read(fad, output_buffer, CDFS_SECTOR_SIZE);
}
