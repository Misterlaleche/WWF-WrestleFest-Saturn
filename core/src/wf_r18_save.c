#include "wf_r18_save.h"
#include <string.h>

_Static_assert(sizeof(wf_r18_save_data_t) ==
               WF_R18_SAVE_RANKING_BYTES + WF_R18_SAVE_OPTIONS_BYTES,
               "save payload must not contain padding");

static uint32_t crc32(const uint8_t *bytes, unsigned count)
{
    uint32_t crc = 0xffffffffu;
    for (unsigned i = 0; i < count; ++i) {
        crc ^= bytes[i];
        for (unsigned bit = 0; bit < 8u; ++bit)
            crc = (crc >> 1) ^ ((crc & 1u) ? 0xedb88320u : 0u);
    }
    return ~crc;
}

void wf_r18_save_defaults(wf_r18_save_data_t *data, const uint8_t *factory_rankings)
{
    if (!data || !factory_rankings) return;
    memcpy(data->rankings, factory_rankings, WF_R18_SAVE_RANKING_BYTES);
    memset(data->options, 0, WF_R18_SAVE_OPTIONS_BYTES);
}

int wf_r18_save_encode(const wf_r18_save_data_t *data,
                       uint8_t image[WF_R18_SAVE_IMAGE_BYTES])
{
    if (!data || !image) return 0;
    memcpy(image, "WFS1", 4);
    image[4] = 0;
    image[5] = 1;
    image[6] = 0;
    image[7] = WF_R18_SAVE_IMAGE_BYTES;
    memcpy(image + 8, data->rankings, WF_R18_SAVE_RANKING_BYTES);
    memcpy(image + 8 + WF_R18_SAVE_RANKING_BYTES, data->options,
           WF_R18_SAVE_OPTIONS_BYTES);
    uint32_t sum = crc32(image, WF_R18_SAVE_IMAGE_BYTES - 4u);
    for (unsigned i = 0; i < 4u; ++i)
        image[WF_R18_SAVE_IMAGE_BYTES - 4u + i] = (uint8_t)(sum >> (24u - 8u * i));
    return 1;
}

int wf_r18_save_decode(const uint8_t image[WF_R18_SAVE_IMAGE_BYTES],
                       wf_r18_save_data_t *data)
{
    if (!image || !data || memcmp(image, "WFS1", 4) || image[4] || image[5] != 1 ||
        image[6] || image[7] != WF_R18_SAVE_IMAGE_BYTES) return 0;
    uint32_t stored = 0;
    for (unsigned i = 0; i < 4u; ++i)
        stored = (stored << 8) | image[WF_R18_SAVE_IMAGE_BYTES - 4u + i];
    if (stored != crc32(image, WF_R18_SAVE_IMAGE_BYTES - 4u)) return 0;
    memcpy(data->rankings, image + 8, WF_R18_SAVE_RANKING_BYTES);
    memcpy(data->options, image + 8 + WF_R18_SAVE_RANKING_BYTES,
           WF_R18_SAVE_OPTIONS_BYTES);
    return 1;
}
