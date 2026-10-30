#ifndef WF_R18_SAVE_SATURN_H
#define WF_R18_SAVE_SATURN_H

#include <stdint.h>
#include "wf_r18_save.h"

typedef enum wf_r18_save_saturn_status {
    WF_R18_SAVE_SATURN_UNAVAILABLE = 0,
    WF_R18_SAVE_SATURN_DEFAULTS = 1,
    WF_R18_SAVE_SATURN_LOADED = 2,
    WF_R18_SAVE_SATURN_CORRUPT = 3,
    WF_R18_SAVE_SATURN_WRITE_ERROR = 4,
    WF_R18_SAVE_SATURN_SAVED = 5
} wf_r18_save_saturn_status_t;

void wf_r18_save_saturn_init(void);
wf_r18_save_saturn_status_t wf_r18_save_saturn_load(uint8_t *factory_rankings);
wf_r18_save_saturn_status_t wf_r18_save_saturn_store(const uint8_t *rankings);
wf_r18_save_saturn_status_t wf_r18_save_saturn_status_get(void);

#endif
