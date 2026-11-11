#ifndef WF_WORKSPACE_H
#define WF_WORKSPACE_H
#include <yaul.h>
#include "wf_video_plan.h"
#include "wf_damage.h"
#include "wf_vdp1_plan.h"
#include "wf_saturn_memory.h"
#include "wf_r17_participant_helpers.h"
#ifdef WF_COMBAT_TAG_TRANSPORT
#include "wp2015_secondary.h"
#endif
#ifdef WF_COMBAT_GLOBAL_RANS
#include "wf_global_rans.h"
_Static_assert(sizeof(wf_global_rans_workspace_t)<=WF_SATURN_HWRAM_HOT_INDEX_BYTES,"Global codec fits the existing hot-index lease");
#endif
/* Exclusive leases. Switching requires completed VDP1/DMA and hidden scanout.
 * No original RAM/runtime/video lives in this workspace. */
#ifndef WF_COMBAT_MODULE
#ifndef WF_FRONT_STAGE_BYTES
#define WF_FRONT_STAGE_BYTES 65536u
#endif
_Static_assert(WF_FRONT_STAGE_BYTES==65536u || WF_FRONT_STAGE_BYTES==131072u ||
               WF_FRONT_STAGE_BYTES==262144u,"Bounded frontend CD staging profile");
#endif
#if defined(WF_FIXED_ATTRACT) && WF_FIXED_ATTRACT
#include "r05/wf_r05_workspace.h"
#endif
typedef union {
#if defined(WF_FIXED_ATTRACT) && WF_FIXED_ATTRACT
 wf_r05_workspace_t attract;
#endif
#ifndef WF_COMBAT_MODULE
 struct {
  wf_video_plan_t plan;
  uint8_t previous_bg[2][4096],previous_text[8192];
  uint32_t previous_palette[1024];
  wf_video_piece_t pieces[1024];
  uint32_t tile_stage[32] __aligned(32);
  uint8_t stage[WF_FRONT_STAGE_BYTES] __aligned(32);
  uint8_t latched_sprites[8192];
  vdp1_cmdt_t commands[1028] __aligned(32);
#ifdef WF_FRONT_RESOURCE_PACK
#ifndef WF_FRONT_PACK_CACHE_BYTES
#define WF_FRONT_PACK_CACHE_BYTES 196608u
#endif
  uint8_t resource_input[WF_FRONT_PACK_CACHE_BYTES] __aligned(32);
  uint8_t resource_header[2048] __aligned(32);
#endif
 } front;
#endif
 struct {
  wf_participant_workspace_t participant;
#ifdef WF_COMBAT_GLOBAL_RANS
  union {
   uint8_t hot_indices[WF_SATURN_HWRAM_HOT_INDEX_BYTES];
   wf_global_rans_workspace_t global_rans;
  } __aligned(32);
#else
  uint8_t hot_indices[WF_SATURN_HWRAM_HOT_INDEX_BYTES] __aligned(32);
#endif
  uint8_t dma_stage[WF_VDP1_STREAM_TILES_MAX*128] __aligned(32);
  wf_vdp1_frame_plan_t plan;
#if defined(WF_COMBAT_MODULE) && !defined(WF_COMBAT_HW1_MANUAL)
  vdp1_cmdt_t commands[WF_VDP1_COMMANDS_MAX+4] __aligned(32);
#else
  vdp1_cmdt_t commands[WF_VDP1_COMMANDS_MAX+WF_DAMAGE_RECTS_MAX+3] __aligned(32);
#endif
#ifdef WF_COMBAT_TAG_TRANSPORT
  wp2015_secondary_workspace_t secondary;
#endif
 } combat;
} wf_workspace_t;
#ifndef WF_COMBAT_MODULE
_Static_assert(offsetof(wf_workspace_t,front.previous_bg)==0xd000,"observer BG offset");
_Static_assert(offsetof(wf_workspace_t,front.previous_text)==0xf000,"observer text offset");
_Static_assert(offsetof(wf_workspace_t,front.previous_palette)==0x11000,"observer palette offset");
_Static_assert(offsetof(wf_workspace_t,front.pieces)==0x12000,"observer sprite offset");
_Static_assert(sizeof(((wf_workspace_t*)0)->front.stage)>=49152,"text write staging capacity");
#endif
#if defined(WF_FRONT_RESOURCE_PACK) && !defined(WF_COMBAT_MODULE)
_Static_assert(WF_FRONT_PACK_CACHE_BYTES==196608u,"Bounded frontend input lease");
_Static_assert(WF_FRONT_STAGE_BYTES==262144u,"Frontend bundle retains caller output stage");
#endif
#if defined(WF_FIXED_ATTRACT) && WF_FIXED_ATTRACT
_Static_assert(sizeof(wf_r05_workspace_t)<=sizeof(((wf_workspace_t*)0)->front),"R05 fits existing frontend lease");
#endif
extern wf_workspace_t wf_workspace;
#define WF_COMBAT_COMMAND_CAPACITY (WF_VDP1_COMMANDS_MAX+WF_DAMAGE_RECTS_MAX+3u)
_Static_assert(WF_COMBAT_COMMAND_CAPACITY<=WF_SATURN_VDP1_CMD_COUNT,"clear and sprite commands fit VRAM lease");
void *wf_workspace_video_stage(void);
void wf_workspace_combat_begin(void);
#endif
