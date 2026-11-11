#include "wf_frontend_save.h"
#include "wf_platform.h"
#include "wf_workspace.h"
#include <string.h>

#ifdef WF_COMBAT_MODULE
#error Frontend backup workspace must not be linked into a combat executable
#endif
_Static_assert(sizeof wf_workspace.front.stage>=65536u,"Frontend BUP staging capacity");
static unsigned leased,initialized,inspected;

static void settle(void){
 vdp1_sync_wait();vdp2_sync_wait();
 for(unsigned i=0;i<3;i++)scu_dma_transfer_wait(i);
 for(unsigned i=0;i<2;i++)cpu_dmac_channel_wait(i);
}

/* Resolve the original adapter's workspace seam without linking the combat
 * renderer. BUP owns only stage; maps, residency metadata and commands remain
 * valid. Its synchronous BIOS calls are excluded from the arcade clock. */
void *wf_r18_present_backup_workspace_begin(size_t bytes){
 if(leased||!bytes||bytes>sizeof wf_workspace.front.stage)return NULL;
 if(!wf_platform_load(true))return NULL;
 leased=1;settle();return wf_workspace.front.stage;
}
void wf_r18_present_backup_workspace_end(void){
 if(!leased)return;
 settle();leased=0;(void)wf_platform_load(false);
}

static int valid(const uint8_t *ram,size_t bytes){
 return ram&&bytes>=0xc2u+WF_R18_SAVE_RANKING_BYTES;
}
wf_r18_save_saturn_status_t wf_frontend_save_init(void){
 inspected=0;initialized=1;wf_r18_save_saturn_init();
 return wf_r18_save_saturn_status_get();
}
wf_r18_save_saturn_status_t wf_frontend_save_load(uint8_t *ram,size_t bytes){
 if(!valid(ram,bytes))return WF_R18_SAVE_SATURN_UNAVAILABLE;
 if(!initialized)(void)wf_frontend_save_init();
 wf_r18_save_saturn_status_t status=wf_r18_save_saturn_load(ram+0xc2u);
 inspected=1;return status;
}
wf_r18_save_saturn_status_t wf_frontend_save_rebind(const uint8_t *ram,size_t bytes){
 if(!valid(ram,bytes))return WF_R18_SAVE_SATURN_UNAVAILABLE;
 uint8_t rankings[WF_R18_SAVE_RANKING_BYTES];
 memcpy(rankings,ram+0xc2u,sizeof rankings);
 (void)wf_frontend_save_init();
 wf_r18_save_saturn_status_t status=wf_r18_save_saturn_load(rankings);
 inspected=1;return status;
}
wf_r18_save_saturn_status_t wf_frontend_save_store(const uint8_t *ram,size_t bytes){
 if(!valid(ram,bytes))return WF_R18_SAVE_SATURN_UNAVAILABLE;
 if(!inspected)(void)wf_frontend_save_rebind(ram,bytes);
 return wf_r18_save_saturn_store(ram+0xc2u);
}
