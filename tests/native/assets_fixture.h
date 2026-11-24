#ifndef WF_HOST_ASSETS_FIXTURE_H
#define WF_HOST_ASSETS_FIXTURE_H
#include "wf_jo_assets.h"
void wf_host_assets_init(void);
void wf_host_assets_play(void);
void wf_host_assets_frame(unsigned frame);
void wf_host_assets_dump(const void *data,size_t bytes);
#endif
