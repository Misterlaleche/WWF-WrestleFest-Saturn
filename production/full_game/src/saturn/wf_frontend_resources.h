#ifndef WF_FRONTEND_RESOURCES_H
#define WF_FRONTEND_RESOURCES_H
#include <stdbool.h>
#include <stdint.h>
extern void *wf_frontend_resources[7];
extern volatile uint32_t wf_assets_stage,wf_assets_error,wf_assets_index,wf_assets_used;
bool wf_frontend_resources_load(void);
#endif
