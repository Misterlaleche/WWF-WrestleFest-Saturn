#ifndef WF_CD_H
#define WF_CD_H
#include "wf_assets.h"
extern volatile uint32_t wf_cd_stage,wf_cd_error,wf_cd_windows,wf_cd_sectors;
bool wf_cd_init(void);
extern const wf_asset_io_t wf_cd_asset_io;
void wf_cd_snapshot(void);
#endif
