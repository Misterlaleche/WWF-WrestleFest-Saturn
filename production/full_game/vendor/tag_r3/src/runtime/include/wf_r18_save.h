#ifndef WF_R18_SAVE_H
#define WF_R18_SAVE_H

#include <stdint.h>

#define WF_R18_SAVE_RANKING_BYTES 132u
#define WF_R18_SAVE_OPTIONS_BYTES 16u
#define WF_R18_SAVE_IMAGE_BYTES 160u

typedef struct wf_r18_save_data {
    uint8_t rankings[WF_R18_SAVE_RANKING_BYTES];
    uint8_t options[WF_R18_SAVE_OPTIONS_BYTES];
} wf_r18_save_data_t;

void wf_r18_save_defaults(wf_r18_save_data_t *data, const uint8_t *factory_rankings);
int wf_r18_save_encode(const wf_r18_save_data_t *data,
                       uint8_t image[WF_R18_SAVE_IMAGE_BYTES]);
int wf_r18_save_decode(const uint8_t image[WF_R18_SAVE_IMAGE_BYTES],
                       wf_r18_save_data_t *data);

#endif
