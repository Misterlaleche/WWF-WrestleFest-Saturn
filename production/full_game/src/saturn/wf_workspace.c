#include "wf_workspace.h"
wf_workspace_t wf_workspace __aligned(32);
#include "wf_saturn_memory.h"
#include <string.h>
static bool combat_lease;
void *wf_workspace_video_stage(void){return combat_lease?(void*)WF_SATURN_LWRAM_VIDEO_WRITES:wf_workspace.front.stage;}
void wf_workspace_combat_begin(void){memset(&wf_workspace,0,sizeof wf_workspace);combat_lease=true;}
