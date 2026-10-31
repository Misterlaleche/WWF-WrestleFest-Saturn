#include <yaul.h>
#include "wf_frontend_resources.h"
#include "wf_cd.h"
#include "asset_manifest.h"
void *wf_frontend_resources[7];
volatile uint32_t wf_assets_stage,wf_assets_error,wf_assets_index,wf_assets_used;
bool wf_frontend_resources_load(void){
 /* Current frontend owns the first128KiB of LWRAM from load through terminal.
  * Later scene transitions must release these pointers before reusing the arena. */
 wf_asset_result_t result;wf_assets_stage=1;
 bool ok=wf_assets_load(frontend_manifest,7,(void*)LWRAM(0),0x20000,wf_frontend_resources,&wf_cd_asset_io,&result);
 wf_assets_error=result.error;wf_assets_index=result.index;wf_assets_used=result.used;
 wf_assets_stage=ok?2:3;return ok;
}
