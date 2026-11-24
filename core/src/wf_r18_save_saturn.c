#include <yaul.h>
#include <string.h>
#include "wf_r18_save.h"
#include "wf_r18_save_saturn.h"
#include "wf_r18_present.h"

/* BUP is synchronous and runs only outside combat. Its code/work area borrows
 * the 32 KiB VDP1 DMA staging buffer, which is idle at those points. */
_Static_assert(sizeof(bup_t) <= WF_VDP1_STREAM_TILES_MAX *
               WF_VDP1_TILE_BYTES_16X16_4BPP, "BUP exceeds VDP1 staging");
static wf_r18_save_data_t save_data;
static uint8_t image[WF_R18_SAVE_IMAGE_BYTES];
static uint8_t saved_image[WF_R18_SAVE_IMAGE_BYTES];
static unsigned ready, saved_valid;
static wf_r18_save_saturn_status_t save_status;
static const char filename[] = "WFESTSAT001";
volatile uint32_t wf_r18_save_probe[8];

static void bup_close(void)
{
    smpc_smc_resenab_call();
    wf_r18_present_backup_workspace_end();
}

static int bup_open(void)
{
    bup_devices_t devices;
    bup_t *area = wf_r18_present_backup_workspace_begin(sizeof(bup_t));
    wf_r18_save_probe[2] = (uint32_t)(uintptr_t)area;
    if (!area) return 0;
    memset(&devices, 0, sizeof devices);
    smpc_smc_resdisa_call();
    bup_init(area, &devices);
    wf_r18_save_probe[3] = (uint32_t)devices.internal.device;
    wf_r18_save_probe[4] = devices.internal.partition_count;
    /* The BIOS returns packed device metadata in this Yaul build (the
     * internal entry appears as 0x00010000). Let its partition operation
     * decide availability instead of comparing the raw enum field. */
    bup_status_t selected = bup_partition_select(BUP_DEVICE_INTERNAL, 0);
    int partition = selected;
    wf_r18_save_probe[5] = (uint32_t)partition;
    if (partition != BUP_OK) {
        bup_close();
        return 0;
    }
    return 1;
}

static bup_dir_date_t save_date(void)
{
    bup_date_t date = { .year = 20, .month = 1, .day = 1 };
    const smpc_time_t *time = smpc_rtc_time_get();
    if (time) {
        smpc_time_dec_t decoded;
        smpc_rtc_time_bcd_from(time, &decoded);
        if (decoded.year >= 1980u && decoded.year <= 2100u &&
            decoded.month >= 1u && decoded.month <= 12u &&
            decoded.day >= 1u && decoded.day <= 31u) {
            date.year = (uint8_t)(decoded.year - 1980u);
            date.month = (uint8_t)decoded.month;
            date.day = decoded.day;
            date.hour = decoded.hours;
            date.minute = decoded.minutes;
            date.week = (uint8_t)decoded.week_day;
        }
    }
    return bup_date_set(&date);
}

void wf_r18_save_saturn_init(void)
{
    bup_stat_t stat;
    ready = saved_valid = 0;
    save_status = WF_R18_SAVE_SATURN_UNAVAILABLE;
    if (!bup_open()) { wf_r18_save_probe[0] = 0; return; }
    wf_r18_save_probe[0] = 1;
    int result = bup_status(BUP_DEVICE_INTERNAL, 0, &stat);
    wf_r18_save_probe[6] = (uint32_t)result;
    bup_close();
    if (result != BUP_OK) return;
    ready = 1;
    save_status = WF_R18_SAVE_SATURN_DEFAULTS;
}

wf_r18_save_saturn_status_t wf_r18_save_saturn_store(const uint8_t *rankings)
{
    bup_directory_t directory;
    bup_stat_t stat;
    /* Preserve an existing unreadable file for recovery through the BIOS. */
    if (!rankings || !ready || save_status == WF_R18_SAVE_SATURN_CORRUPT)
        return save_status;
    memcpy(save_data.rankings, rankings, WF_R18_SAVE_RANKING_BYTES);
    if (!wf_r18_save_encode(&save_data, image)) return WF_R18_SAVE_SATURN_WRITE_ERROR;
    if (saved_valid && !memcmp(image, saved_image, sizeof image)) return save_status;
    if (!bup_open()) {
        save_status = WF_R18_SAVE_SATURN_UNAVAILABLE;
        return save_status;
    }
    if (bup_status(BUP_DEVICE_INTERNAL, sizeof image, &stat) != BUP_OK) {
        bup_close();
        save_status = WF_R18_SAVE_SATURN_WRITE_ERROR;
        return save_status;
    }
    memset(&directory, 0, sizeof directory);
    memcpy(directory.filename, filename, sizeof filename);
    memcpy(directory.comment, "WRESTLEFST", 11);
    directory.language = BUP_LANG_ENGLISH;
    directory.date = save_date();
    directory.data_size = sizeof image;
    int written = bup_write(BUP_DEVICE_INTERNAL, &directory, image,
                            BUP_WRITE_MODE_OVERWRITE) == BUP_OK &&
                  bup_verify(BUP_DEVICE_INTERNAL, filename, image) == BUP_OK;
    bup_close();
    if (!written) {
        save_status = WF_R18_SAVE_SATURN_WRITE_ERROR;
        return save_status;
    }
    memcpy(saved_image, image, sizeof image);
    saved_valid = 1;
    save_status = WF_R18_SAVE_SATURN_SAVED;
    return save_status;
}

wf_r18_save_saturn_status_t wf_r18_save_saturn_load(uint8_t *factory_rankings)
{
    bup_directory_t directory;
    wf_r18_save_data_t decoded;
    if (!factory_rankings) return WF_R18_SAVE_SATURN_UNAVAILABLE;
    wf_r18_save_defaults(&save_data, factory_rankings);
    saved_valid = 0;
    if (!ready) return save_status;
    if (!bup_open()) {
        wf_r18_save_probe[1] = 0;
        save_status = WF_R18_SAVE_SATURN_UNAVAILABLE;
        return save_status;
    }
    wf_r18_save_probe[1] = 1;
    int32_t count = bup_dir(BUP_DEVICE_INTERNAL, filename, 1, &directory);
    wf_r18_save_probe[7] = (uint32_t)count;
    if (count == 0) {
        bup_close();
        save_status = WF_R18_SAVE_SATURN_DEFAULTS;
        return wf_r18_save_saturn_store(factory_rankings);
    }
    int valid = count == 1 && directory.data_size == sizeof image &&
        bup_read(BUP_DEVICE_INTERNAL, filename, image) == BUP_OK &&
        wf_r18_save_decode(image, &decoded);
    bup_close();
    if (!valid) {
        save_status = WF_R18_SAVE_SATURN_CORRUPT;
        return save_status;
    }
    save_data = decoded;
    memcpy(factory_rankings, save_data.rankings, WF_R18_SAVE_RANKING_BYTES);
    memcpy(saved_image, image, sizeof image);
    saved_valid = 1;
    save_status = WF_R18_SAVE_SATURN_LOADED;
    return save_status;
}

wf_r18_save_saturn_status_t wf_r18_save_saturn_status_get(void)
{
    return save_status;
}
