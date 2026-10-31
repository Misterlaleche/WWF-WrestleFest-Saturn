#ifndef WF_WORKSPACE_H
#define WF_WORKSPACE_H
#include <yaul.h>
#include "wf_video_plan.h"
#include "wf_vdp1_plan.h"
#include "wf_r17_participant_helpers.h"
/* Exclusive leases. Switching requires completed VDP1/DMA and hidden scanout.
 * No original RAM/runtime/video lives in this workspace. */
typedef union {
 struct {
  wf_video_plan_t plan;
  uint8_t previous_bg[2][4096],previous_text[8192];
  uint32_t previous_palette[1024];
  wf_video_piece_t pieces[1024];
  uint32_t tile_stage[32] __aligned(32);
  uint8_t stage[65536] __aligned(32);
  uint8_t latched_sprites[8192];
  vdp1_cmdt_t commands[1028] __aligned(32);
 } front;
 struct {
  wf_participant_workspace_t participant;
  uint8_t hot_indices[0x14000] __aligned(32);
  uint8_t dma_stage[WF_VDP1_STREAM_TILES_MAX*128] __aligned(32);
  wf_vdp1_frame_plan_t plan;
  vdp1_cmdt_t commands[WF_VDP1_COMMANDS_MAX+4] __aligned(32);
 } combat;
} wf_workspace_t;
_Static_assert(offsetof(wf_workspace_t,front.previous_bg)==0xd000,"observer BG offset");
_Static_assert(offsetof(wf_workspace_t,front.previous_text)==0xf000,"observer text offset");
_Static_assert(offsetof(wf_workspace_t,front.previous_palette)==0x11000,"observer palette offset");
_Static_assert(offsetof(wf_workspace_t,front.pieces)==0x12000,"observer sprite offset");
_Static_assert(sizeof(((wf_workspace_t*)0)->front.stage)>=49152,"text write staging capacity");
extern wf_workspace_t wf_workspace;
void *wf_workspace_video_stage(void);
void wf_workspace_combat_begin(void);
#endif
