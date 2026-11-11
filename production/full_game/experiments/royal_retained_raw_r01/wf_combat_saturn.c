#if defined(WF_TAG_PREFETCH) && WF_TAG_PREFETCH
#include "wf_tag_prefetch_saturn.h"
#endif
#include "wf_combat_saturn.h"
#include "wf_combat.h"
#include "wf_combat_diagnostic.h"
#include "wf_combat_assets.h"
#include "wf_input_bus.h"
#include "wf_platform.h"
#if defined(WF_ROM_HANDOFF) && WF_ROM_HANDOFF
#include "wf_rom_handoff_saturn.h"
#endif
#include "wf_live_video.h"
#include "wf_workspace.h"
#include "wf_cd.h"
#ifdef WF_ALPHA_RETURN
#include "wf_finale_boundary.h"
#endif
#include "wf_r18_cdfs.h"
#include "wf_r18_present.h"
#include "wf_r18_ring.h"
#include "wf_r18_hud.h"
#include "wf_r18_display.h"
#include "wf_r18_transition.h"
#include "wf_r18_match_result.h"
#include "wf_scene_checkpoint.h"
#include "wf_scene_background.h"
#include "wf_saturn_memory.h"
#include <string.h>
#include <dram-cart.h>
#ifdef WF_COMBAT_BANKED_TRANSPORT
#include "wp2015_display_pipeline.h"
#include "wf_saturn_yaul_backend.h"
#endif
#ifdef WF_COMBAT_TAG_TRANSPORT
#include "wp2015_secondary.h"
#ifdef WF_COMBAT_TAG_STARTUP_STREAM
#include "wp2015_secondary_memory.h"
#endif
#include "wf_rom_preload_pack.h"
#endif
#if defined(WF_COMBAT_TAG_CROP_STAGE) && WF_COMBAT_TAG_CROP_STAGE
#if !defined(WF_COMBAT_TAG_ATLAS_ARCHIVE) || !WF_COMBAT_TAG_ATLAS_ARCHIVE || !defined(WF_COMBAT_MODULE) || !defined(WF_COMBAT_CROPPED_CART) || !defined(WF_COMBAT_HW1_BANKED) || defined(WF_COMBAT_TAG_TRANSPORT) || defined(WF_COMBAT_BASE_ROYAL) || defined(WF_COMBAT_GLOBAL_RANS) || (defined(WF_COMBAT_ROYAL_FULL_CROP) && (!defined(WF_COMBAT_SHARED_CART_ROYAL) || !WF_COMBAT_SHARED_CART_ROYAL))
#error TAG crop staging requires a TAG archive CART module and explicit shared Royal opt-in
#endif
#include "wf_tag_crop_stage.h"
#endif
#if defined(WF_COMBAT_HUD_WINDOW) && (!defined(WF_COMBAT_HW1_BANKED) || defined(WF_COMBAT_BASE_ROYAL) || defined(WF_COMBAT_TAG_TRANSPORT))
#error HUD window opt-in requires the dedicated CART module
#endif
#if defined(WF_COMBAT_TAG_TRANSPORT) || defined(WF_COMBAT_HUD_WINDOW)
#include "wf_load_window.h"
/* Startup borrowers use this lease sequentially and close before drawing. */
static wf_load_window_t startup_load_window;
#endif
static wf_combat_t combat;
static wf_session_t *session;
static wf_board_t *board;
static wf_r18_display_t display;
static wf_r18_transition_context_t transition;
#ifdef WF_COMBAT_TAG_TRANSPORT
static uint32_t secondary_transition_token;
volatile uint32_t wf_secondary_transition_attempts,wf_secondary_transition_restored;
static wf_rom_preload_pack_t *rom_preload_pack;
volatile uint32_t wf_secondary_load_requests,wf_secondary_load_reads;
volatile uint32_t wf_rom_preload_requests,wf_rom_preload_reads,wf_rom_preload_error;
#endif
static uint8_t priority;
static bool raw_atlas;
static unsigned asset_mask,asset_royal;
static bool asset_profile;
#ifdef WF_COMBAT_GLOBAL_RANS
#if !defined(WF_COMBAT_CROPPED_CART) || !defined(WF_COMBAT_HW1_BANKED) || defined(WF_COMBAT_BASE_ROYAL) || defined(WF_COMBAT_TAG_TRANSPORT) || defined(WF_COMBAT_PROFILE_ARCHIVE) || defined(WF_COMBAT_RETAINED_CROP_POOL) || defined(WF_COMBAT_SHARED_CROP_POOL) || defined(WF_COMBAT_GROUPED_CROP_POOL)
#error Complete sprite residency requires its dedicated CART variant
#endif
volatile uint32_t wf_combat_global_error,wf_combat_global_tile;
bool wf_combat_global_file_bytes(uint32_t*bytes){return bytes&&!wf_cd_asset_io.size(NULL,"CROPPOOL.BIN",bytes);}
#endif
#ifdef WF_COMBAT_PROFILE_ARCHIVE
#ifndef WF_COMBAT_RETAINED_CROP_POOL
#error Profile archive requires the dedicated retained CART module
#endif
#include "wf_profile_archive.h"
static wf_profile_archive_t profile_archive;
volatile uint32_t wf_combat_archive_key,wf_combat_archive_error;
static int archive_source(void*user,uint32_t at,void*dst,size_t bytes){
 (void)user;return wf_cd_asset_io.read(NULL,"PROFILES.BIN",at,dst,bytes);
}
#endif
#ifdef WF_COMBAT_ROYAL_FULL_CROP
#if !defined(WF_COMBAT_CROPPED_CART) || !defined(WF_COMBAT_HW1_BANKED) || defined(WF_COMBAT_BASE_ROYAL) || defined(WF_COMBAT_TAG_TRANSPORT) || defined(WF_COMBAT_GLOBAL_RANS)
#error Full Royal requires its dedicated CART module
#endif
#include "wf_royal_full_crop.h"
static bool royal_full_mode;
extern volatile uint32_t wf_combat_dependency;
volatile uint32_t wf_combat_full_error,wf_combat_full_tile,wf_combat_full_block;
volatile uint32_t wf_combat_full_hits,wf_combat_full_misses,wf_combat_full_decodes,wf_combat_full_decoded_bytes;
/* Observe once per frame/startup boundary; no diagnostic stores per tile. */
static void royal_full_observe(void){
 wf_combat_full_hits=wf_royal_full_crop.hits;wf_combat_full_misses=wf_royal_full_crop.misses;
 wf_combat_full_decodes=wf_royal_full_crop.decodes;wf_combat_full_decoded_bytes=wf_royal_full_crop.decoded_bytes;
}
void wf_combat_full_crop_failure(uint32_t error,uint32_t tile,uint32_t block){
 if(!wf_combat_full_error){wf_combat_full_error=error;wf_combat_full_tile=tile;wf_combat_full_block=block;}
 wf_combat_dependency=0xa600u|wf_combat_full_error;
 wf_royal_full_crop.active=false;royal_full_observe();
}
/* Keep writes on the audited P1 lease; publish only its immutable P0 alias.
 * The synchronous reader has finished before this one-per-load boundary. */
const uint8_t*wf_combat_full_crop_read_alias(void *destination,size_t bytes){
 if(!royal_full_mode||!wf_cart_valid||wf_cart_id!=0x5cu||wf_cart_bytes!=0x400000u||
    (uintptr_t)destination!=0x22400000u||bytes<4u||bytes>0x400000u||(bytes&31u))return NULL;
 __asm__ volatile("" ::: "memory");
 uint32_t drained=*(volatile uint32_t*)((uint8_t*)destination+bytes-4u);
 (void)drained;
 __asm__ volatile("" ::: "memory");
 cpu_cache_purge();
 __asm__ volatile("" ::: "memory");
 return (const uint8_t*)0x02400000u;
}
static void royal_full_begin(void){
 wf_r18_full_crop_reset();
 royal_full_mode=asset_royal&&wf_cart_valid&&wf_cart_bytes==0x400000u;
#ifdef WF_DISABLE_ROYAL_FULL_CROP_EXPERIMENT
 royal_full_mode=false; /* Test R23A/R22S lossless retained Royal path. */
#endif
 wf_combat_full_error=0;wf_combat_full_tile=wf_combat_full_block=UINT32_MAX;royal_full_observe();
}
#endif
static const char *asset_name(const char *name,char out[13]){
#ifdef WF_COMBAT_ROYAL_FULL_CROP
 if(royal_full_mode&&(!strcmp(name,"HOTSPR.BIN")||!strcmp(name,"ROYRAW.BIN")))return "ROYRAW.BIN";
#endif
#ifdef WF_COMBAT_GLOBAL_RANS
 if(!strcmp(name,"ROYRAW.BIN")||!strcmp(name,"TAGRAW.BIN"))return "CROPPOOL.BIN";
#endif
 if(!asset_profile)return name;
 char kind=!strcmp(name,"HOTSPR.BIN")?'H':!strcmp(name,"ROYSPR.BIN")?'Z':
  !strcmp(name,"ROYRAW.BIN")?'U':!strcmp(name,"CROP.BIN")?'C':
  !strcmp(name,"TAGEX.BIN")?'S':!strcmp(name,"SPRWARM.BIN")?'W':0;
 return kind&&wf_combat_asset_name(out,asset_royal,asset_mask,kind)?out:name;
}
/* TAG retains its cartridge ROM lease after3MiB. The HW1 cartridge profile
 * relocates that ROM to LWRAM below and can use all4MiB for its raw atlas. */
enum { CART_ROM_OFFSET=0x300000u,CART_ROM_BYTES=0x80000u };
static const uint8_t *crop_hints;
#if (defined(WF_COMBAT_SHARED_CROP_POOL) || defined(WF_COMBAT_GROUPED_CROP_POOL) || defined(WF_COMBAT_RETAINED_CROP_POOL)) && !defined(WF_COMBAT_CROPPED_CART)
#error Shared crop pool requires the dedicated cropped CART module
#endif
#if (defined(WF_COMBAT_SHARED_CROP_POOL) + defined(WF_COMBAT_GROUPED_CROP_POOL) + defined(WF_COMBAT_RETAINED_CROP_POOL)) > 1
#error Choose one crop pool transport
#endif
#ifdef WF_COMBAT_CROPPED_CART
#if !defined(WF_COMBAT_HW1_BANKED) || defined(WF_COMBAT_BASE_ROYAL) || defined(WF_COMBAT_TAG_TRANSPORT)
#error R19C requires the dedicated HW1 banked CART module
#endif
extern bool wf_r18_tile_packed(uint16_t tile);
const uint8_t*wf_combat_crop_hints(void){return crop_hints;}
#endif
uint16_t wf_combat_crop_hint(uint16_t tile){
#ifdef WF_COMBAT_CROPPED_CART
 if(!crop_hints)return 0xffffu;
 uint16_t crop=(uint16_t)((unsigned)crop_hints[(unsigned)tile*2]<<8|crop_hints[(unsigned)tile*2+1]);
 if(wf_r18_tile_packed(tile))crop|=0x8000u;
 return crop;
#else
 return crop_hints?(uint16_t)((unsigned)crop_hints[(unsigned)tile*2]<<8|crop_hints[(unsigned)tile*2+1]):0xffffu;
#endif
}
volatile uint32_t wf_combat_rom_resident;
volatile uint32_t wf_combat_finished;
volatile uint32_t wf_combat_failure_site;
volatile uint32_t wf_combat_warm_tiles;
volatile uint32_t wf_handoff_rng,wf_handoff_frame,wf_handoff_ids[9];
volatile uint32_t wf_combat_stage,wf_combat_frame,wf_combat_ack,wf_combat_error,wf_combat_dependency,wf_combat_sprites,wf_combat_rng,wf_combat_audio_count,wf_combat_audio_last,wf_combat_work,wf_combat_wait;
/* Read-only evidence, in FRT/32 ticks (uint16 delta). Fields/CD do not wrap
 * at the FRT period: long reads remain identifiable instead of hidden. */
volatile uint32_t wf_combat_profile_sequence,wf_combat_profile[8],wf_combat_profile_total[8];
volatile uint32_t wf_combat_cd_rom,wf_combat_cd_sprites;
volatile uint32_t wf_combat_cd_scene,wf_combat_cd_other;
#if defined(WF_COMBAT_TAG_ATLAS_ARCHIVE) && WF_COMBAT_TAG_ATLAS_ARCHIVE
#if !defined(WF_COMBAT_CROPPED_CART) || !defined(WF_COMBAT_HW1_BANKED) || defined(WF_COMBAT_TAG_TRANSPORT) || defined(WF_COMBAT_BASE_ROYAL) || defined(WF_COMBAT_GLOBAL_RANS)
#error TAG archive requires the dedicated R19C CART module
#endif
#include "wf_tag_atlas_archive.h"
#if defined(WF_COMBAT_TAG_STARTUP_LZ4) && WF_COMBAT_TAG_STARTUP_LZ4
#include "wf_tag_startup_lz4.h"
#include "wp1980_lz4.h"
static wf_tag_lz4_t tag_startup_lz4;
volatile uint32_t wf_tag_start_io_fields,wf_tag_start_decode_fields,wf_tag_start_total_fields;
volatile uint32_t wf_tag_start_bind_fields,wf_tag_start_rom_fields,wf_tag_start_scene_fields;
/* Only the synchronous startup decoder may use cached cart aliases. The CD
 * writer has returned before entry; purge its previous input incarnation.
 * SH2 writes are through to the same physical cart. Drain the final write and
 * discard cached aliases before the original P1 provider/scene loaders run. */
static int tag_startup_cached_decode(const uint8_t*source,size_t in,uint8_t*output,size_t out){
 uintptr_t src=(uintptr_t)source,dst=(uintptr_t)output;
 if(src<0x22400000u||src>=0x22800000u||(src&31u)||!in||in>65536u||in>0x22800000u-src||
    dst<0x22400000u||dst>src||(dst&31u)||!out||out>65536u||out>src-dst)return -1;
 uint32_t started=wf_fields;
 __asm__ volatile("" ::: "memory");
 uint32_t drained=*(volatile uint32_t*)(src+((in-1u)&~3u));(void)drained;
 cpu_cache_purge();
 __asm__ volatile("" ::: "memory");
#if defined(WF_COMBAT_TAG_STARTUP_STAGE) && WF_COMBAT_TAG_STARTUP_STAGE
 /* Startup-only lease: the cropped hot index is not populated until the
  * subsequent original atlas bind. No renderer batch exists here. Decode
  * matches in HWRAM, then copy the completed block to its final P1 range. */
 _Static_assert(sizeof wf_workspace.combat.hot_indices>=65536u,"Startup block lease");
 int rc=wp1980_lz4_decode((const uint8_t*)(src&~(uintptr_t)0x20000000u),in,
     wf_workspace.combat.hot_indices,out);
 if(rc==0)memcpy(output,wf_workspace.combat.hot_indices,out);
#else
 int rc=wp1980_lz4_decode((const uint8_t*)(src&~(uintptr_t)0x20000000u),in,
     (uint8_t*)(dst&~(uintptr_t)0x20000000u),out);
#endif
 __asm__ volatile("" ::: "memory");
 drained=*(volatile uint32_t*)((dst+out-1u)&~(uintptr_t)3u);(void)drained;
 cpu_cache_purge();
 __asm__ volatile("" ::: "memory");
 wf_tag_start_decode_fields+=wf_fields-started;
 return rc;
}
#endif
#include "wf_tag_scene_cache.h"
static wf_tag_atlas_archive_t tag_archive;
static wf_tag_scene_cache_t tag_scenes;
static bool tag_preload_route;
volatile uint32_t wf_combat_tag_key,wf_combat_tag_error,wf_combat_tag_tile;
/* Dependency0xa501 capacity,2 scratch,3 archive file,4 missing key,
 *5 malformed/I/O,6 scene load,7 virtual span/state,8 original sprite miss. */
static void tag_preload_reset(void){
#if defined(WF_COMBAT_TAG_STARTUP_LZ4) && WF_COMBAT_TAG_STARTUP_LZ4
 wf_tag_start_io_fields=wf_tag_start_decode_fields=wf_tag_start_total_fields=0;
 wf_tag_start_bind_fields=wf_tag_start_rom_fields=wf_tag_start_scene_fields=0;
 memset(&tag_startup_lz4,0,sizeof tag_startup_lz4);
#endif
 memset(&tag_archive,0,sizeof tag_archive);memset(&tag_scenes,0,sizeof tag_scenes);
 tag_preload_route=false;wf_combat_tag_key=wf_combat_tag_error=0;wf_combat_tag_tile=UINT32_MAX;
}
static int tag_preload_fail(unsigned error){
#if defined(WF_COMBAT_TAG_STARTUP_LZ4) && WF_COMBAT_TAG_STARTUP_LZ4
 memset(&tag_startup_lz4,0,sizeof tag_startup_lz4);
#endif
 if(!wf_combat_tag_error)wf_combat_tag_error=error;
 wf_combat_dependency=0xa500u|wf_combat_tag_error;
 memset(&tag_archive,0,sizeof tag_archive);memset(&tag_scenes,0,sizeof tag_scenes);
 return -1;
}
/* Startup source callbacks deliberately bypass the virtual resolver. */
static int tag_archive_source(void*user,uint32_t at,void*dst,size_t n){
 (void)user;
#if defined(WF_COMBAT_TAG_STARTUP_LZ4) && WF_COMBAT_TAG_STARTUP_LZ4
 uint32_t start=wf_fields;int rc=
#if defined(WF_TAG_PREFETCH) && WF_TAG_PREFETCH
 wf_tag_prefetch_cart_read(at,dst,n);
#else
 wf_cd_asset_io.read(NULL,"TAGRAW.BIN",at,dst,n);
#endif
 wf_tag_start_io_fields+=wf_fields-start;return rc;
#else
 return wf_cd_asset_io.read(NULL,"TAGRAW.BIN",at,dst,n);
#endif
}
static int tag_scene_source(void*user,const char*name,uint32_t at,void*dst,size_t n){
 (void)user;return wf_cd_asset_io.read(NULL,name,at,dst,n);
}
static int tag_scene_size(void*user,const char*name,uint32_t*n){
 (void)user;return wf_cd_asset_io.size(NULL,name,n);
}
/* Validation reads an immutable alias only; the presenter retains its P1
 * gameplay pointer after the original full checksum/map/hint validation. */
const uint8_t*wf_combat_tag_validation_alias(void*destination,size_t bytes){
 if(asset_royal||!tag_preload_route)return destination;
 if(!tag_archive.active||tag_archive.bytes<2048u||bytes!=tag_archive.bytes-2048u||
    !wf_cart_valid||wf_cart_id!=0x5cu||wf_cart_bytes!=0x400000u||
    (uintptr_t)destination!=0x22400000u||bytes<WF_CROPPED_ATLAS_MAP_BYTES||
    bytes>0x400000u||(bytes&3u))return NULL;
 __asm__ volatile("" ::: "memory");
 uint32_t drained=*(volatile uint32_t*)((uint8_t*)destination+bytes-4u);(void)drained;
 cpu_cache_purge();
 __asm__ volatile("" ::: "memory");
 return (const uint8_t*)0x02400000u;
}
static int tag_preload_open(size_t capacity){
 tag_preload_reset();tag_preload_route=true;
 /* Resolve again from this combat's actual actors; legacy P markers never
  * authorize a default atlas or retain the preceding match's binding. */
 asset_mask=wf_combat_asset_mask(session->runtime.actors,9,sizeof session->runtime.actors[0]);
 wf_combat_tag_key=asset_mask;
 if(capacity!=0x400000u)return tag_preload_fail(1);
 uint32_t bytes;
 if(wf_cd_asset_io.size(NULL,"TAGRAW.BIN",&bytes))return tag_preload_fail(3);
 void*scratch=wf_r18_present_backup_workspace_begin(WF_TAG_ATLAS_ARCHIVE_SCRATCH);
 if(!scratch)return tag_preload_fail(2);
#if defined(WF_COMBAT_TAG_STARTUP_LZ4) && WF_COMBAT_TAG_STARTUP_LZ4
 (void)wf_tag_atlas_archive_open;
 uint32_t started=wf_fields;
 int rc=wf_tag_lz4_open(&tag_startup_lz4,tag_archive_source,NULL,bytes,asset_mask,
  dram_cart_area_get(),capacity,scratch,WF_TAG_ATLAS_ARCHIVE_SCRATCH,tag_startup_cached_decode);
 wf_tag_start_total_fields=wf_fields-started;
 if(rc==1)tag_archive=(wf_tag_atlas_archive_t){.read=wf_tag_lz4_read,.user=&tag_startup_lz4,
  .offset=0,.bytes=tag_startup_lz4.memory+2048u,.key=asset_mask,.active=true};
#else
 int rc=wf_tag_atlas_archive_open(&tag_archive,tag_archive_source,NULL,bytes,asset_mask,
  scratch,WF_TAG_ATLAS_ARCHIVE_SCRATCH,capacity);
#endif
 wf_r18_present_backup_workspace_end();
 if(rc!=1)return tag_preload_fail(rc==0?4:5);
 return 1;
}
static int tag_preload_scenes(void*cart,size_t capacity){
 if(!tag_archive.active||tag_archive.bytes<2048u||capacity!=0x400000u||
#if defined(WF_COMBAT_TAG_MATCH_SCENES) && WF_COMBAT_TAG_MATCH_SCENES
    wf_tag_scene_cache_open_match(&tag_scenes,cart,capacity,tag_archive.bytes-2048u,
      session->runtime.g.progression_0162,session->runtime.g.global_007e,
#else
    wf_tag_scene_cache_open(&tag_scenes,cart,capacity,tag_archive.bytes-2048u,
#endif
      tag_scene_source,tag_scene_size,NULL)!=1)return tag_preload_fail(6);
 return 1;
}
#endif
#if defined(WF_COMBAT_TAG_SMALL_CART_SCENES) && WF_COMBAT_TAG_SMALL_CART_SCENES
#ifndef WF_COMBAT_TAG_TRANSPORT
#error Small cartridge scene lease is only for the base TAG transport
#endif
#include "wf_tag_scene_cache.h"
_Static_assert(WF_TAG_SCENE_COUNT==5u,"Small cart TAG must include original scene6");
static wf_tag_scene_cache_t tag_small_scenes;
static int tag_small_scene_source(void*u,const char*n,uint32_t at,void*d,size_t bytes){
 (void)u;return wf_cd_asset_io.read(NULL,n,at,d,bytes);
}
static int tag_small_scene_size(void*u,const char*n,uint32_t*bytes){
 (void)u;return wf_cd_asset_io.size(NULL,n,bytes);
}
static int tag_small_scenes_open(void){
 memset(&tag_small_scenes,0,sizeof tag_small_scenes);
 if(asset_royal||!wf_cart_valid||wf_cart_id!=0x5au||wf_cart_bytes!=0x100000u)return 0;
 /* This TAG profile never places ROM or sprites in a1MiB cart. Lease only
  * the first512KiB of the SDK's validated contiguous view; no bank crossing.
  * The largest original closure is three scenes (338112 aligned bytes). */
 return wf_tag_scene_cache_open_match(&tag_small_scenes,dram_cart_area_get(),0x80000u,0,
  session->runtime.g.progression_0162,session->runtime.g.global_007e,
  tag_small_scene_source,tag_small_scene_size,NULL);
}
#endif
#ifdef WF_COMBAT_GLOBAL_RANS
#ifdef WF_GLOBAL_RANS_PROFILE
volatile uint32_t wf_global_rans_cost[8];
#endif
#ifdef WF_GLOBAL_RANS_TRACE
volatile uint16_t wf_global_rans_trace_tiles[WF_GLOBAL_RANS_TRACE_CAPACITY];
volatile uint32_t wf_global_rans_trace_count,wf_global_rans_trace_overflow;
#endif
void wf_combat_global_failure(int code,uint32_t tile){
 if(!wf_combat_global_error){wf_combat_global_error=(uint32_t)-code;wf_combat_global_tile=tile;}
 wf_combat_dependency=0xa400u|wf_combat_global_error;
}
#endif
__attribute__((cold,noinline)) void wf_failure_combat_read(wf_failure_combat_t *out){
 wf_failure_combat_from_runtime(out,session?&session->runtime:NULL);
 if(!out->valid)return;
 out->ack=wf_combat_ack;out->site=wf_combat_failure_site;
 out->rom_address=session->video.failed_address;out->bad_slot=wf_failure_actor_slot(combat.gameplay.last_blocked_phase,combat.gameplay.last_blocked_slot);
#ifdef WF_COMBAT_BANKED_TRANSPORT
 out->render_error=wp2015_display_observer[8];
#else
 out->render_error=UINT32_MAX;
#endif
 out->secondary_error=out->secondary_tile=UINT32_MAX;
#ifdef WF_COMBAT_ROYAL_FULL_CROP
 if(wf_combat_full_error){out->secondary_error=wf_combat_full_error;out->secondary_tile=wf_combat_full_tile;}
#endif
#if defined(WF_COMBAT_TAG_ATLAS_ARCHIVE) && WF_COMBAT_TAG_ATLAS_ARCHIVE
 if(wf_combat_tag_error){out->secondary_error=wf_combat_tag_error;out->secondary_tile=wf_combat_tag_tile;}
#endif
#ifdef WF_COMBAT_GLOBAL_RANS
 if(wf_combat_global_error){out->secondary_error=wf_combat_global_error;out->secondary_tile=wf_combat_global_tile;}
#endif
#ifdef WF_COMBAT_TAG_TRANSPORT
 if(wp2015_secondary_active){
  out->secondary_error=wp2015_secondary_active->state.last_error;
  out->secondary_tile=wp2015_secondary_active->state.last_tile;
 }
#endif
}
#if WF_COMBAT_TRACE_ASSETS
volatile uint32_t wf_asset_trace_count,wf_asset_trace_overflow,wf_asset_trace_other;
#if WF_COMBAT_TRACE_FINE
uint8_t wf_asset_trace_tiles[8192];
#elif WF_COMBAT_TRACE_COARSE
/* Diagnostic Royal keeps its production cache budget. One bit per16-tile
 * source sector bounds tracing to512B; export conservatively includes all
 * original tiles in each touched sector. This is not a gameplay cache. */
uint8_t wf_asset_trace_sectors[512];
#else
uint16_t wf_asset_trace_ids[768];
#endif
uint8_t wf_asset_trace_rom[64];
static void trace_asset(const char*name,uint32_t at,size_t n){
 if(!strcmp(name,"SPRITES.PAK"))for(unsigned id=at/128u;id<(at+n+127u)/128u;id++){
#if WF_COMBAT_TRACE_FINE
  unsigned mask=1u<<(id&7u);
  if(id>=65536u){wf_asset_trace_overflow=1;continue;}
  if(!(wf_asset_trace_tiles[id>>3]&mask)){wf_asset_trace_tiles[id>>3]|=(uint8_t)mask;++wf_asset_trace_count;}
#elif WF_COMBAT_TRACE_COARSE
  unsigned sector=id/16u,mask=1u<<(sector&7u);
  if(sector>=4096u){wf_asset_trace_overflow=1;continue;}
  if(!(wf_asset_trace_sectors[sector>>3]&mask)){wf_asset_trace_sectors[sector>>3]|=(uint8_t)mask;++wf_asset_trace_count;}
#else
  unsigned i=0;while(i<wf_asset_trace_count&&wf_asset_trace_ids[i]!=id)++i;
  if(i==wf_asset_trace_count){if(i<768u){wf_asset_trace_ids[i]=(uint16_t)id;++wf_asset_trace_count;}else wf_asset_trace_overflow=1;}
#endif
 }else if(!strcmp(name,"ROM68K.BIN"))for(unsigned p=at/1024u;p<(at+n+1023u)/1024u;p++){
  if(p<512u)wf_asset_trace_rom[p>>3]|=(uint8_t)(1u<<(p&7u));
 }else ++wf_asset_trace_other;
}
/* Diagnostic only: include cached demands as well as CD misses. No writes
 * to the game state; release modules have neither calls nor these maps. */
void wf_trace_rom(uint32_t at,size_t bytes){if(wf_combat_stage>=4)trace_asset("ROM68K.BIN",at,bytes);}
void wf_trace_tile(uint16_t tile){if(wf_combat_stage>=4)trace_asset("SPRITES.PAK",(uint32_t)tile*128u,128u);}
#endif
#if WF_NATIVE_PROFILE
static wf_phase_binding_t measured_bindings[36];
volatile uint32_t wf_phase_cycles[36],wf_phase_calls[36],wf_phase_owners[36];
static wf_status_t measured_phase(wf_runtime_t*r,uint32_t address,void*u){
 wf_phase_binding_t*b=u;unsigned i=(unsigned)(b-measured_bindings),start=cpu_frt_count_get();
 wf_status_t st=b->fn(r,address,b->user);
 wf_phase_cycles[i]+=(uint16_t)(cpu_frt_count_get()-start);++wf_phase_calls[i];return st;
}
#endif
#if defined(WF_COMBAT_TAG_HOT_STARTUP_STREAM) && WF_COMBAT_TAG_HOT_STARTUP_STREAM
#if !defined(WF_COMBAT_TAG_TRANSPORT) || !defined(WF_COMBAT_TAG_STARTUP_STREAM)
#error HOT startup stream requires TAG startup transport
#endif
/* Scalars only: the existing load window retains the resolved physical name. */
static unsigned hot_startup_lease,hot_startup_stream;
static uint32_t hot_startup_end,hot_startup_source_end;
static int hot_startup_error;
static int hot_startup_read(const char*,uint32_t,void*,size_t);
#endif
static int file_read(const char*name,uint32_t at,void*dst,size_t n){
#if defined(WF_ROM_HANDOFF) && WF_ROM_HANDOFF
 if(wf_rom_handoff_cart_read(name,at,dst,n))return 0;
#endif
#ifdef WF_COMBAT_ROYAL_FULL_CROP
 if(royal_full_mode&&!strcmp(name,"SPRITES.PAK")){
  wf_combat_full_crop_failure(WF_FULL_CROP_ERROR_INACTIVE,at/128u,UINT32_MAX);return -1;
 }
#endif
#if defined(WF_COMBAT_TAG_HOT_STARTUP_STREAM) && WF_COMBAT_TAG_HOT_STARTUP_STREAM
 if(hot_startup_lease||hot_startup_error)return hot_startup_read(name,at,dst,n);
#endif
#ifdef WF_COMBAT_TAG_TRANSPORT
 if(rom_preload_pack&&!strcmp(name,"ROM68K.BIN")){
  ++wf_rom_preload_requests;
  return wf_rom_preload_pack_read(rom_preload_pack,at,dst,n);
 }
#endif
#if WF_COMBAT_TRACE_ASSETS
 if(wf_combat_stage>=4)trace_asset(name,at,n);
#endif
 if(!strcmp(name,"HOTSPR.BIN")){
  if(session->runtime.g.mode_0161&1)name=raw_atlas?"ROYRAW.BIN":"ROYSPR.BIN";
  else if(raw_atlas)name="TAGRAW.BIN";
 }
#ifdef WF_COMBAT_PROFILE_ARCHIVE
 if(profile_archive.active){
  if(!strcmp(name,"ROYRAW.BIN"))return wf_profile_archive_read(&profile_archive,0,at,dst,n);
  if(!strcmp(name,"SPRWARM.BIN"))return wf_profile_archive_read(&profile_archive,1,at,dst,n);
 }
#endif
#if defined(WF_COMBAT_TAG_ATLAS_ARCHIVE) && WF_COMBAT_TAG_ATLAS_ARCHIVE
 if(tag_preload_route){
  if(!strcmp(name,"TAGRAW.BIN")){
   int rc=wf_tag_atlas_archive_read(&tag_archive,at,dst,n);
   return rc?tag_preload_fail(7):0;
  }
  if(wf_tag_scene_index(name)>=0){
   int rc=wf_tag_scene_cache_read(&tag_scenes,name,at,dst,n);
   return rc?tag_preload_fail(7):0;
  }
  if(!strcmp(name,"SPRITES.PAK")){
   if(wf_combat_tag_tile==UINT32_MAX)wf_combat_tag_tile=at/128u;
   return tag_preload_fail(8);
  }
 }
#endif
#if defined(WF_COMBAT_TAG_SMALL_CART_SCENES) && WF_COMBAT_TAG_SMALL_CART_SCENES
 if(tag_small_scenes.active&&wf_tag_scene_index(name)>=0)
  return wf_tag_scene_cache_read(&tag_small_scenes,name,at,dst,n);
#endif
 char resolved[13];name=asset_name(name,resolved);
 uint32_t before=wf_cd_sectors;int rc=wf_cd_asset_io.read(NULL,name,at,dst,n);
 if(wf_combat_stage>=4){
  if(!strcmp(name,"ROM68K.BIN"))wf_combat_cd_rom+=wf_cd_sectors-before;
  else if(!strcmp(name,"SPRITES.PAK"))wf_combat_cd_sprites+=wf_cd_sectors-before;
  else if(!strncmp(name,"R18",3))wf_combat_cd_scene+=wf_cd_sectors-before;
  else wf_combat_cd_other+=wf_cd_sectors-before;
 }
 return rc;
}
static int file_size(const char*name,uint32_t*n){
#ifdef WF_COMBAT_ROYAL_FULL_CROP
 if(royal_full_mode){
  if(!strcmp(name,"SPRITES.PAK")){wf_combat_full_crop_failure(WF_FULL_CROP_ERROR_INACTIVE,UINT32_MAX,UINT32_MAX);return -1;}
  if(!strcmp(name,"HOTSPR.BIN")||!strcmp(name,"ROYRAW.BIN")){
   int rc=wf_cd_asset_io.size(NULL,"ROYRAW.BIN",n);
   if(rc)wf_combat_full_crop_failure(WF_FULL_CROP_ERROR_ARGUMENT,UINT32_MAX,UINT32_MAX);
   return rc;
  }
 }
#endif
#if defined(WF_COMBAT_TAG_ATLAS_ARCHIVE) && WF_COMBAT_TAG_ATLAS_ARCHIVE
 if(tag_preload_route){
  if(!strcmp(name,"TAGRAW.BIN")||!strcmp(name,"HOTSPR.BIN")){
   if(!n||!tag_archive.active)return tag_preload_fail(7);
   *n=tag_archive.bytes;return 0;
  }
  if(wf_tag_scene_index(name)>=0){
   int rc=wf_tag_scene_cache_size(&tag_scenes,name,n);
   return rc?tag_preload_fail(7):0;
  }
 }
#endif
#ifdef WF_COMBAT_PROFILE_ARCHIVE
 if(profile_archive.active&&n){
  if(!strcmp(name,"ROYRAW.BIN")){*n=profile_archive.selector_bytes;return 0;}
  if(!strcmp(name,"SPRWARM.BIN")){*n=profile_archive.warm_bytes;return 0;}
 }
#endif
#if defined(WF_COMBAT_TAG_SMALL_CART_SCENES) && WF_COMBAT_TAG_SMALL_CART_SCENES
 if(tag_small_scenes.active&&wf_tag_scene_index(name)>=0)
  return wf_tag_scene_cache_size(&tag_small_scenes,name,n);
#endif
 char resolved[13];return wf_cd_asset_io.size(NULL,asset_name(name,resolved),n);
}
#if defined(WF_COMBAT_SHARED_CROP_POOL) || defined(WF_COMBAT_GROUPED_CROP_POOL) || defined(WF_COMBAT_RETAINED_CROP_POOL)
bool wf_combat_crop_pool_info(uint32_t*pool,uint32_t*selector,uint32_t*key){
 if(!pool||!selector||!key||file_size("CROPPOOL.BIN",pool)||
    file_size(asset_royal?"ROYRAW.BIN":"TAGRAW.BIN",selector))return false;
 *key=(asset_royal<<12)|asset_mask;return true;
}
#endif
#ifdef WF_COMBAT_TAG_TRANSPORT
#ifdef WF_COMBAT_TAG_STARTUP_STREAM
extern int wf_r18_startup_stream_begin(const char*,uint32_t);
extern int wf_r18_startup_stream_end(void);
static __attribute__((cold,noinline)) int startup_stream_status(int rc){
 if(rc&&!wf_cd_error)wf_cd_error=(uint32_t)rc;return rc;
}
#if defined(WF_COMBAT_TAG_HOT_STARTUP_STREAM) && WF_COMBAT_TAG_HOT_STARTUP_STREAM
static __attribute__((cold,noinline)) int hot_startup_fail(int rc){
 if(rc&&!hot_startup_error)hot_startup_error=rc;
 return startup_stream_status(hot_startup_error);
}
static __attribute__((cold,noinline)) int hot_startup_close(int rc){
 (void)hot_startup_fail(rc);
 if(hot_startup_lease){
  if(startup_load_window.busy)return hot_startup_fail(WF_LOAD_WINDOW_STATE);
  if(hot_startup_stream){
   (void)hot_startup_fail(wf_r18_startup_stream_end());hot_startup_stream=0;
  }
  wf_load_window_close(&startup_load_window);
  wf_r18_present_backup_workspace_end();hot_startup_lease=0;
 }
 return hot_startup_error;
}
static __attribute__((cold,noinline)) int hot_startup_source(const char*name,uint32_t at,void*dst,size_t bytes){
 if(hot_startup_error)return hot_startup_error;
 if(!hot_startup_lease||strcmp(name,startup_load_window.name)||
    dst!=startup_load_window.buffer||at!=hot_startup_source_end)
  return hot_startup_fail(WF_LOAD_WINDOW_STATE);
 /* End the fully drained prefix before the original CDFS partial tail.
  * Small files never arm a zero-length Play. No callback through file_read. */
 if(bytes!=WF_LOAD_WINDOW_BYTES&&hot_startup_stream){
  int rc=wf_r18_startup_stream_end();hot_startup_stream=0;
  if(rc)return hot_startup_fail(rc);
 }
 int rc=wf_cd_asset_io.read(NULL,name,at,dst,bytes);
 if(rc||hot_startup_error)return hot_startup_fail(rc);
 hot_startup_source_end=at+(uint32_t)bytes;return 0;
}
static __attribute__((cold,noinline)) int hot_startup_read(const char*name,uint32_t at,void*dst,size_t bytes){
 if(hot_startup_error)return hot_startup_error;
 if(!hot_startup_lease)return hot_startup_fail(WF_LOAD_WINDOW_STATE);
 if(!name||strcmp(name,"HOTSPR.BIN")||at<hot_startup_end)
  return hot_startup_fail(WF_LOAD_WINDOW_ARGUMENT);
 int rc=wf_load_window_read(&startup_load_window,startup_load_window.name,at,dst,bytes);
 if(rc||hot_startup_error)return hot_startup_fail(rc);
 /* Ordinary HOT reads skip header/index padding. Only a successful nonempty
  * logical demand advances completion; a zero-byte EOF read proves nothing. */
 if(bytes)hot_startup_end=at+(uint32_t)bytes;
 return 0;
}
static __attribute__((cold,noinline)) int hot_startup_open(uint32_t bytes){
 if(hot_startup_error)return hot_startup_error;
 if(hot_startup_lease||startup_load_window.active||startup_load_window.busy||
    wf_combat_stage!=2u||!bytes)return hot_startup_fail(WF_LOAD_WINDOW_STATE);
 void *scratch=wf_r18_present_backup_workspace_begin(WF_LOAD_WINDOW_BYTES);
 if(!scratch)return hot_startup_fail(WF_LOAD_WINDOW_STATE);
 hot_startup_lease=1;hot_startup_end=hot_startup_source_end=0;
 /* open saves an aliased name before clearing the window. Resolve exactly
  * once into its existing name field; no additional name or data buffer. */
 const char *physical=asset_name("HOTSPR.BIN",startup_load_window.name);
 int rc=wf_load_window_open(&startup_load_window,scratch,WF_LOAD_WINDOW_BYTES,
                           physical,bytes,hot_startup_source);
 uint32_t prefix=bytes&~(uint32_t)(WF_LOAD_WINDOW_BYTES-1u);
 if(!rc&&prefix){
  hot_startup_stream=1;rc=wf_r18_startup_stream_begin(startup_load_window.name,prefix);
 }
 return rc?hot_startup_close(rc):0;
}
__attribute__((cold,noinline)) int wf_tag_hot_startup_finish(void){
 if(!hot_startup_lease||startup_load_window.busy)return hot_startup_fail(WF_LOAD_WINDOW_STATE);
 if(hot_startup_end!=startup_load_window.file_bytes||
    hot_startup_source_end!=startup_load_window.file_bytes)
  return hot_startup_close(WF_LOAD_WINDOW_STATE);
 return hot_startup_close(0);
}
static __attribute__((cold,noinline)) wf_status_t hot_startup_load(uint32_t bytes){
 if(hot_startup_open(bytes))return WF_BLOCKED;
 wf_status_t status=wf_r18_hot_sprites_load();
 /* All parser/early returns release ownership. Success also requires that
  * the projected loader already closed BEFORE publication/CROP/rank. */
 int rc=hot_startup_close(status==WF_OK&&!hot_startup_lease?0:WF_LOAD_WINDOW_STATE);
 return status==WF_OK&&!rc?WF_OK:WF_BLOCKED;
}
#endif
/* Only the physical span ends here: the32KiB source window and batch7 loan
 * remain active, including the cached first24452bytes of the future tail. */
__attribute__((cold,noinline)) int wf_tag_secondary_startup_stream_finish(void){
 return startup_stream_status(wf_r18_startup_stream_end());
}
#endif
#ifdef WF_COMBAT_TAG_BATCH7
/* Init-only loan. No draw batch may begin until the source-window lease
 * ends; display_begin then writes control/sprite/END commands before DMA. */
__attribute__((cold,noinline)) void *wf_tag_secondary_startup_buffer(void){
 _Static_assert(sizeof wf_workspace.combat.commands>=7u*2352u,"Seven sectors fit existing commands");
 _Static_assert(__alignof__(wf_workspace.combat.commands)>=32u,"Startup FIFO buffer alignment");
 return wf_combat_stage==3u&&startup_load_window.active?wf_workspace.combat.commands:NULL;
}
#endif
static int rom_preload_source(const char*name,uint32_t at,void*dst,size_t bytes){
#ifdef WF_COMBAT_TAG_STARTUP_STREAM
 /* R's final nonfull window is deliberately legacy, including its sector
  * cache tail. Close the fully consumed physical prefix BEFORE delegating. */
 if(bytes!=WF_LOAD_WINDOW_BYTES){int rc=startup_stream_status(wf_r18_startup_stream_end());if(rc)return rc;}
#endif
 ++wf_rom_preload_reads;
 return file_read(name,at,dst,bytes);
}
static int secondary_window_source(const char*name,uint32_t at,void*dst,size_t bytes){
 ++wf_secondary_load_reads;
 return file_read(name,at,dst,bytes);
}
static int secondary_window_read(const char*name,uint32_t at,void*dst,size_t bytes){
 ++wf_secondary_load_requests;
 if(strcmp(name,WP2015_SECONDARY_FILE))return WF_LOAD_WINDOW_ARGUMENT;
 return wf_load_window_read(&startup_load_window,startup_load_window.name,at,dst,bytes);
}
static int secondary_load(void *tail,uint32_t bytes){
 /* The tail lease is already acquired. HOT/CROP/rank are complete, no
  * batch has begun, and this staging lease ends before display_begin. */
 void *scratch=wf_r18_present_backup_workspace_begin(WF_LOAD_WINDOW_BYTES);
 if(!scratch)return WF_LOAD_WINDOW_STATE;
 char resolved[13];const char *physical=asset_name(WP2015_SECONDARY_FILE,resolved);
 wf_secondary_load_requests=wf_secondary_load_reads=0;
 int rc=wf_load_window_open(&startup_load_window,scratch,WF_LOAD_WINDOW_BYTES,
                           physical,bytes,secondary_window_source);
#ifdef WF_COMBAT_TAG_STARTUP_STREAM
 enum {prefix=(WP2015_SECONDARY_HEADER_BYTES+WP2015_SECONDARY_METADATA_BYTES+
  WP2015_SECONDARY_MODEL_BYTES+sizeof wf_workspace.combat.secondary.resident+
  WP2015_SECONDARY_LWRAM_BYTES+WP2015_SECONDARY_VRAM_BYTES+32767u)&~32767u};
 _Static_assert(prefix==491520u,"Audited pre-P23 source prefix");
 if(!rc)rc=startup_stream_status(wf_r18_startup_stream_begin(physical,prefix));
#endif
 if(!rc)rc=wp2015_secondary_load(&wf_workspace.combat.secondary,tail,
                               WP2015_SECONDARY_LWRAM_BYTES,bytes,secondary_window_read);
#ifdef WF_COMBAT_TAG_STARTUP_STREAM
 int stream_rc=startup_stream_status(wf_r18_startup_stream_end());if(!rc)rc=stream_rc;
#endif
 wf_load_window_close(&startup_load_window);
 wf_r18_present_backup_workspace_end();
 return rc;
}
#endif
#if defined(WF_COMBAT_TAG_TRANSPORT) || defined(WF_COMBAT_HUD_WINDOW)
static int hud_window_read(const char*name,uint32_t at,void*dst,size_t bytes){
 return wf_load_window_read(&startup_load_window,name,at,dst,bytes);
}
static wf_status_t hud_load(wf_r18_video_t *video){
 uint32_t bytes;
 if(file_size("FONT8.BIN",&bytes)||bytes!=WF_SATURN_FONT_BYTES)return WF_BLOCKED;
 /* The binder consumes its callback synchronously. Its original 2 KiB
  * destinations and VDP2 stores remain unchanged; no batch is active. */
 void *scratch=wf_r18_present_backup_workspace_begin(WF_LOAD_WINDOW_BYTES);
 if(!scratch)return WF_BLOCKED;
 int rc=wf_load_window_open(&startup_load_window,scratch,WF_LOAD_WINDOW_BYTES,
                           "FONT8.BIN",bytes,file_read);
#ifdef WF_COMBAT_TAG_STARTUP_STREAM
 if(!rc)rc=startup_stream_status(wf_r18_startup_stream_begin("FONT8.BIN",bytes));
#endif
 wf_status_t status=rc?WF_BLOCKED:wf_r18_hud_bind(video,hud_window_read);
#ifdef WF_COMBAT_TAG_STARTUP_STREAM
 if(startup_stream_status(wf_r18_startup_stream_end()))status=WF_BLOCKED;
#endif
 wf_load_window_close(&startup_load_window);
 wf_r18_present_backup_workspace_end();
 return status;
}
#endif
/* This code runs only under the explicit load barrier. Keep its validation
 * compact without changing the optimization of the per-frame owner. */
static __attribute__((cold,noinline)) wf_status_t roster_pages(void){
 if(!asset_profile)return wf_combat_preload_pages(asset_royal);
 char name[13];uint32_t bytes;uint8_t data[8+264*2];uint16_t pages[264];
 if(!wf_combat_asset_name(name,asset_royal,asset_mask,'P')||file_size(name,&bytes)||
    bytes<10||bytes>sizeof data||file_read(name,0,data,bytes)||memcmp(data,"R18Q",4))return WF_BLOCKED;
 unsigned main=(unsigned)data[4]<<8|data[5],fast=(unsigned)data[6]<<8|data[7];
 if(!main||main>WF_SATURN_LWRAM_ROM_BYTES/1024u||fast>WF_R18_ROM_EXTRA_CAPACITY||bytes!=8u+2u*(main+fast))return WF_BLOCKED;
 for(unsigned i=0;i<main+fast;i++){
  pages[i]=(uint16_t)((unsigned)data[8+i*2]<<8|data[9+i*2]);
  if(pages[i]>=512u||(i&&i!=main&&pages[i]<=pages[i-1]))return WF_BLOCKED;
 }
 for(unsigned i=main;i<main+fast;i++)for(unsigned j=0;j<main;j++)if(pages[i]==pages[j])return WF_BLOCKED;
#ifdef WF_COMBAT_TAG_TRANSPORT
 /* P remains the authority for the unchanged present APIs and cache slots.
  * A declared profile must carry its matching pack; a bad pack is fatal.
  * No drawing/HOT load is active while dma_stage holds this temporary lease. */
 if(!wf_combat_asset_name(name,asset_royal,asset_mask,'R'))return WF_BLOCKED;
 uint32_t pack_bytes;
 if(file_size(name,&pack_bytes))return WF_BLOCKED;
 void *scratch=wf_r18_present_backup_workspace_begin(WF_LOAD_WINDOW_BYTES);
 if(!scratch)return WF_BLOCKED;
 wf_rom_preload_pack_t pack={0};
 wf_rom_preload_requests=wf_rom_preload_reads=wf_rom_preload_error=0;
 int rc=wf_load_window_open(&startup_load_window,scratch,WF_LOAD_WINDOW_BYTES,
                           name,pack_bytes,rom_preload_source);
#ifdef WF_COMBAT_TAG_STARTUP_STREAM
 if(!rc)rc=startup_stream_status(wf_r18_startup_stream_begin(name,pack_bytes&~32767u));
#endif
 if(!rc)rc=wf_rom_preload_pack_open(&pack,&startup_load_window,
     (uint16_t)((asset_royal<<12)|asset_mask),data,bytes,
     WF_SATURN_LWRAM_ROM_BYTES/1024u,WF_R18_ROM_EXTRA_CAPACITY);
 wf_status_t status=WF_BLOCKED;
 if(!rc){
  rom_preload_pack=&pack;
  status=wf_r18_rom_pages_preload(pages,main);
  if(status==WF_OK&&fast)status=wf_r18_rom_extra_pages_preload(pages+main,fast);
  rc=status==WF_OK?wf_rom_preload_pack_finish(&pack):
     (pack.error?pack.error:WF_ROM_PRELOAD_PACK_STATE);
  rom_preload_pack=NULL;
 }
 wf_rom_preload_pack_close(&pack);
#ifdef WF_COMBAT_TAG_STARTUP_STREAM
 int stream_rc=startup_stream_status(wf_r18_startup_stream_end());if(!rc)rc=stream_rc;
#endif
 wf_load_window_close(&startup_load_window);
 wf_r18_present_backup_workspace_end();
 wf_rom_preload_error=(uint32_t)rc;
 return rc?WF_BLOCKED:status;
#else
 wf_status_t status=wf_r18_rom_pages_preload(pages,main);
 return status==WF_OK&&fast?wf_r18_rom_extra_pages_preload(pages+main,fast):status;
#endif
}
static wf_status_t input(wf_runtime_t*r __unused,uint32_t at,uint16_t*out,void*u __unused){
 return wf_input_bus_word(at,out,combat.gameplay.input_hw_low,wf_r18_rom_read,NULL);
}
static wf_status_t audio(wf_runtime_t*r __unused,uint16_t command,void*u __unused){
 /* Audio disabled by the phase spec; retain every original command. */
 wf_combat_audio_last=command;++wf_combat_audio_count;
 return wf_board_write(board,0x14000c,command,2)?WF_OK:WF_BLOCKED;
}
static wf_status_t wait_ticks(wf_runtime_t*r __unused,uint16_t raw,void*u __unused){
 for(unsigned i=0;i<=(unsigned)raw;i++)if(!wf_platform_tick_wait())return WF_BLOCKED;
 return WF_OK;
}
static wf_status_t world_animation(wf_runtime_t*r,uint8_t record[4],uint8_t bank,void*u __unused){return wf_r18_world_288f0(r,record,bank,&combat.world);}
static wf_status_t world_scroll(wf_runtime_t*r,uint8_t side,uint16_t direction,void*u __unused){return wf_r18_world_26b68(r,side,direction,&combat.world);}
static wf_status_t reset(wf_runtime_t*r,uint8_t royal,void*u){
#ifdef WF_COMBAT_TAG_TRANSPORT
 secondary_transition_token=wp2015_secondary_transition_begin(r->g.global_007e);
 if(secondary_transition_token)++wf_secondary_transition_attempts;
#endif
 wf_status_t st=wf_r18_transition_reset(r,royal,u);
#ifdef WF_COMBAT_TAG_TRANSPORT
 if(st!=WF_OK){(void)wp2015_secondary_transition_end(secondary_transition_token,r->g.global_007e,0);secondary_transition_token=0;}
#endif
 return st;
}
static wf_status_t barrier(wf_runtime_t*r,uint8_t royal,void*u){
 if(!wf_platform_load(true)){
#ifdef WF_COMBAT_TAG_TRANSPORT
  (void)wp2015_secondary_transition_end(secondary_transition_token,r->g.global_007e,0);secondary_transition_token=0;
#endif
  return WF_BLOCKED;
 }
 wf_status_t st=wf_r18_transition_barrier(r,royal,u);
 bool loaded=wf_platform_load(false);
#ifdef WF_COMBAT_TAG_TRANSPORT
 if(wp2015_secondary_transition_end(secondary_transition_token,r->g.global_007e,loaded&&st==WF_OK))++wf_secondary_transition_restored;
 secondary_transition_token=0;
#endif
 return loaded?st:WF_BLOCKED;
}
static bool palettes_ring(void){
 wf_r18_video_t*v=&session->video;
 if(wf_r18_hud_commit(v)!=WF_OK)return false;
 for(unsigned i=WF_R18_PALETTE_TEXT;i<=WF_R18_PALETTE_BG;i++)if(wf_r18_palettes_commit(v,i)!=WF_OK)return false;
 return !wf_r18_ring_update_deferred((int16_t)display.scroll[0],(int16_t)(2048-display.scroll[1]),
  (int16_t)display.scroll[2],(int16_t)(2048-display.scroll[3]),priority);
}
__attribute__((cold,noinline)) bool wf_combat_saturn_begin(wf_session_t*s,wf_board_t*b){
#if defined(WF_COMBAT_TAG_ATLAS_ARCHIVE) && WF_COMBAT_TAG_ATLAS_ARCHIVE
 tag_preload_reset();
#endif
 if(!s||!b||s->phase!=WF_SESSION_GAMEPLAY||wf_combat_stage)return false;
 session=s;board=b;wf_combat_stage=1;
#if defined(WF_COMBAT_TAG_SMALL_CART_SCENES) && WF_COMBAT_TAG_SMALL_CART_SCENES
 memset(&tag_small_scenes,0,sizeof tag_small_scenes);
#endif
 asset_royal=s->runtime.g.mode_0161&1;
 asset_mask=wf_combat_asset_mask(s->runtime.actors,9,sizeof s->runtime.actors[0]);
#ifdef WF_COMBAT_ROYAL_FULL_CROP
 royal_full_begin();
#endif
 char profile_name[13];uint32_t profile_size;
 asset_profile=
#ifdef WF_COMBAT_ROYAL_FULL_CROP
  !royal_full_mode&&
#endif
  wf_combat_asset_name(profile_name,asset_royal,asset_mask,'P')&&
  !wf_cd_asset_io.size(NULL,profile_name,&profile_size);
#ifdef WF_COMBAT_PROFILE_ARCHIVE
 memset(&profile_archive,0,sizeof profile_archive);
 wf_combat_archive_key=wf_combat_archive_error=0;
#endif
 wf_handoff_rng=s->runtime.g.rng_state;wf_handoff_frame=s->runtime.g.frame_counter;
 for(unsigned i=0;i<9;i++)wf_handoff_ids[i]=wf_be16(s->runtime.actors+i,WF_A_CHARACTER);
#ifdef WF_COMBAT_TAG_TRANSPORT
 raw_atlas=false;
#elif defined(WF_COMBAT_MODULE)
 raw_atlas=wf_cart_valid&&wf_cart_bytes==0x400000u;
#else
 raw_atlas=(s->runtime.g.mode_0161&1)&&wf_cart_valid&&wf_cart_bytes==0x400000u;
#endif
 if(!wf_platform_load(true))return false;
#ifndef WF_COMBAT_MODULE
 wf_live_video_finish();
 /* Retain completed manual swaps. Bounded transparent polygons clear both
  * previous footprints; VBlank erase alone was incomplete at320x240. */
 vdp1_sync_mode_set(VDP1_SYNC_MODE_CHANGE_ONLY);
#endif
 const wf_combat_io_t io={wf_r18_rom_read,input,audio,wait_ticks,wf_r18_present_batch,reset,barrier};
 if(wf_combat_join(&combat,s,&io)!=WF_OK)goto failed;
#if WF_NATIVE_PROFILE
 for(unsigned i=0;i<36;i++){
  measured_bindings[i]=combat.bindings[i];wf_phase_owners[i]=combat.bindings[i].rom_addr;
  combat.bindings[i].fn=measured_phase;combat.bindings[i].user=measured_bindings+i;
 }
#endif
 combat.gameplay.branchb_288f0=world_animation;combat.gameplay.branchb_26b68=world_scroll;
 priority=b->control[17];
 /* PROGRAM/BACK cease to be readers before native caches take their LWRAM. */
 wf_workspace_combat_begin();
 bool resident=wf_cart_valid&&wf_cart_bytes>=CART_ROM_OFFSET+CART_ROM_BYTES;
 void*rom_cache=resident?(uint8_t*)dram_cart_area_get()+CART_ROM_OFFSET:(void*)WF_SATURN_LWRAM_ROM_CACHE;
 void*tile_cache=(void*)WF_SATURN_LWRAM_TILE_CACHE;size_t tile_bytes=WF_SATURN_LWRAM_TILE_BYTES,atlas_capacity=CART_ROM_OFFSET;
 (void)atlas_capacity;
#if defined(WF_COMBAT_HW1_BANKED) && !defined(WF_COMBAT_BASE_ROYAL)
 if(resident&&raw_atlas){
  /* Full4MiB cart atlas, immutable512KiB ROM in the former sprite lease.
   * The former ROM lease holds128KiB crop hints plus81KiB cold tiles.
   * None overlaps the ring, video writes, commands, or capsule. */
  _Static_assert(WF_SATURN_LWRAM_TILE_BYTES>=CART_ROM_BYTES,"Cart ROM lease");
  _Static_assert(WF_SATURN_LWRAM_ROM_BYTES>=131072u+128u,"Cart crop/cold lease");
  rom_cache=(void*)WF_SATURN_LWRAM_TILE_CACHE;
  tile_cache=(void*)(WF_SATURN_LWRAM_ROM_CACHE+131072u);tile_bytes=WF_SATURN_LWRAM_ROM_BYTES-131072u;
  atlas_capacity=wf_cart_bytes;
#if defined(WF_ROM_HANDOFF) && WF_ROM_HANDOFF
  /* Consume and close the top-of-CART lease before CROP_STAGE can reuse it. */
  wf_rom_handoff_cart_restore();
#if defined(WF_TAG_PREFETCH) && WF_TAG_PREFETCH
  wf_tag_prefetch_cart_open(session);
#endif
#endif
  uint8_t header[8];uint32_t bytes;
#if defined(WF_COMBAT_TAG_CROP_STAGE) && WF_COMBAT_TAG_CROP_STAGE
  if(!asset_royal){
   void*cart=dram_cart_area_get();
   uintptr_t staging;crop_hints=NULL;
   /* Top-of-CART lease closes synchronously before ROM/atlas/scene loads.
    * Read unchanged CROPALL at offset0, validate, then copy final hints. */
   if(file_size("CROPALL.BIN",&bytes)||bytes!=131080u||
      !(
#if defined(WF_TAG_PREFETCH) && WF_TAG_PREFETCH
      wf_tag_prefetch_cart_crop(&staging)||
#endif
      wf_tag_crop_stage_lease((uintptr_t)cart,wf_cart_bytes,
       WF_SATURN_LWRAM_ROM_CACHE,131072u,&staging))||
      !wf_tag_crop_stage_load((void*)staging,WF_TAG_CROP_STAGE_BYTES,
       (void*)WF_SATURN_LWRAM_ROM_CACHE,131072u,bytes,file_read))goto failed;
   crop_hints=(const uint8_t*)WF_SATURN_LWRAM_ROM_CACHE;
  }else{
#endif
  if(file_size("CROPALL.BIN",&bytes)||bytes!=131080u||file_read("CROPALL.BIN",0,header,8)||memcmp(header,"R18A",4)||
     file_read("CROPALL.BIN",8,(void*)WF_SATURN_LWRAM_ROM_CACHE,131072u))goto failed;
  const uint8_t *hints=(const uint8_t*)WF_SATURN_LWRAM_ROM_CACHE;uint32_t hash=2166136261u;
  for(unsigned i=0;i<131072u;i++)hash=(hash^hints[i])*16777619u;
  if(hash!=((uint32_t)header[4]<<24|(uint32_t)header[5]<<16|(uint32_t)header[6]<<8|header[7]))goto failed;
  for(unsigned i=0;i<65536u;i++){
   unsigned meta=hints[i*2],height=hints[i*2+1],top=meta&31u;
   if((meta&128u)||top>16u||height>16u||top+height>16u||((meta&64u)&&!(meta&32u)))goto failed;
  }
  crop_hints=hints;
#if defined(WF_COMBAT_TAG_CROP_STAGE) && WF_COMBAT_TAG_CROP_STAGE
  }
#endif
 }
#endif
 wf_r18_present_init(file_read,rom_cache,resident?CART_ROM_BYTES:WF_SATURN_LWRAM_ROM_BYTES,
  tile_cache,tile_bytes);
#ifdef WF_COMBAT_ROYAL_FULL_CROP
 if(royal_full_mode){
  _Static_assert(WF_SATURN_LWRAM_ROM_CACHE+131072u==WF_ROYAL_FULL_COLD,"Royal hints/cold boundary");
  _Static_assert(WF_ROYAL_FULL_COLD_END<=WF_SATURN_LWRAM_VIDEO_WRITES,"Royal cold/video boundary");
  _Static_assert(WF_SATURN_LWRAM_TILE_CACHE==WF_ROYAL_FULL_ROM,"Royal ROM start");
  _Static_assert(WF_ROYAL_FULL_ROM+CART_ROM_BYTES==WF_ROYAL_FULL_ROM_END,"Royal ROM extent");
  _Static_assert(WF_ROYAL_FULL_TAIL_END<=0x00300000u,"Royal ROM tail boundary");
  if(!wf_r18_full_crop_configure((uint8_t*)WF_ROYAL_FULL_COLD,5u*16384u,
       (uint8_t*)WF_ROYAL_FULL_TAIL,7u*16384u))goto failed;
 }
#endif
 wf_combat_stage=2;
#ifdef WF_COMBAT_PROFILE_ARCHIVE
 if(resident&&raw_atlas&&asset_royal&&!asset_profile
#ifdef WF_COMBAT_ROYAL_FULL_CROP
    &&!royal_full_mode
#endif
 ){
  uint32_t archive_bytes;
  if(!wf_cd_asset_io.size(NULL,"PROFILES.BIN",&archive_bytes)){
   /* No drawing or prewarm batch is active. Return this DMA staging lease
    * before any native renderer call; only the checked warm list persists. */
   void*scratch=wf_r18_present_backup_workspace_begin(WF_PROFILE_ARCHIVE_SCRATCH);
   if(!scratch){wf_combat_archive_error=1;wf_combat_dependency=0xa301u;goto failed;}
   int rc=wf_profile_archive_open(&profile_archive,archive_source,NULL,archive_bytes,
       0x1000u|asset_mask,scratch,WF_PROFILE_ARCHIVE_SCRATCH);
   wf_r18_present_backup_workspace_end();
   if(rc<0){wf_combat_archive_error=(uint32_t)-rc;wf_combat_dependency=0xa300u|(uint32_t)-rc;goto failed;}
   if(rc)wf_combat_archive_key=0x1000u|asset_mask;
  }
 }
#endif
#if defined(WF_COMBAT_TAG_ATLAS_ARCHIVE) && WF_COMBAT_TAG_ATLAS_ARCHIVE
 if(!asset_royal&&tag_preload_open(resident&&raw_atlas?wf_cart_bytes:0u)!=1)goto failed;
#endif
#if defined(WF_COMBAT_TAG_STARTUP_LZ4) && WF_COMBAT_TAG_STARTUP_LZ4
 uint32_t tag_rom_started=wf_fields;
#endif
 if(resident){
  if(wf_r18_rom_preload()!=WF_OK)goto failed;
  wf_combat_rom_resident=1;
 }else{
  if(roster_pages()!=WF_OK)goto failed;
 }
#if defined(WF_COMBAT_TAG_STARTUP_LZ4) && WF_COMBAT_TAG_STARTUP_LZ4
 wf_tag_start_rom_fields=wf_fields-tag_rom_started;
#endif
#if defined(WF_COMBAT_TAG_SMALL_CART_SCENES) && WF_COMBAT_TAG_SMALL_CART_SCENES
 if(tag_small_scenes_open()<0)goto failed;
#endif
 uint32_t size;uint8_t*scene=(uint8_t*)WF_SATURN_LWRAM_SCENE;
 unsigned world=s->runtime.g.global_007e;const char*map=world==0?"R18RING.BIN":world==1?"R18MAP1.BIN":world==2?"R18MAP2.BIN":world==5?"R18MAP5.BIN":NULL;
#if defined(WF_COMBAT_TAG_SCENE6) && WF_COMBAT_TAG_SCENE6
 if(world==6)map="R18MAP6.BIN";
#endif
#ifdef WF_COMBAT_BASE_ROYAL
 /* Bind the ring's real character extent before leasing its unused VDP2
  * bytes for immutable compressed sprites. Display remains hidden. */
 if(!map||file_size(map,&size)||size>WF_SATURN_LWRAM_SCENE_BYTES||file_read(map,0,scene,size)||
  (((unsigned)scene[6]<<8)|scene[7])!=world||wf_r18_ring_bind(scene,size))goto failed;
#endif
 const char*atlas=(s->runtime.g.mode_0161&1)?(raw_atlas?"ROYRAW.BIN":"ROYSPR.BIN"):(raw_atlas?"TAGRAW.BIN":"HOTSPR.BIN");
 if(file_size(atlas,&size))goto failed;
#if defined(WF_COMBAT_TAG_STARTUP_LZ4) && WF_COMBAT_TAG_STARTUP_LZ4
 uint32_t tag_bind_started=wf_fields;
#endif
#ifdef WF_COMBAT_TAG_TRANSPORT
#if defined(WF_COMBAT_TAG_HOT_STARTUP_STREAM) && WF_COMBAT_TAG_HOT_STARTUP_STREAM
 wf_status_t loaded=hot_startup_load(size);
#else
 wf_status_t loaded=wf_r18_hot_sprites_load();
#endif
#else
 wf_status_t loaded=raw_atlas?wf_r18_hot_sprites_load_raw(dram_cart_area_get(),atlas_capacity):wf_r18_hot_sprites_load();
#endif
#if defined(WF_COMBAT_TAG_STARTUP_LZ4) && WF_COMBAT_TAG_STARTUP_LZ4
 wf_tag_start_bind_fields=wf_fields-tag_bind_started;
#endif
#ifdef WF_COMBAT_ROYAL_FULL_CROP
 if((loaded!=WF_OK||size!=wf_r18_hot_sprites_bytes())&&royal_full_mode&&!wf_combat_full_error)
  wf_combat_full_crop_failure(WF_FULL_CROP_ERROR_HEADER,UINT32_MAX,UINT32_MAX);
#endif
 if(loaded!=WF_OK||size!=wf_r18_hot_sprites_bytes())goto failed;
#if defined(WF_COMBAT_TAG_ATLAS_ARCHIVE) && WF_COMBAT_TAG_ATLAS_ARCHIVE
#if defined(WF_COMBAT_TAG_STARTUP_LZ4) && WF_COMBAT_TAG_STARTUP_LZ4
 uint32_t tag_scene_started=wf_fields;
#endif
 if(tag_preload_route&&tag_preload_scenes(dram_cart_area_get(),wf_cart_bytes)!=1)goto failed;
#if defined(WF_COMBAT_TAG_STARTUP_LZ4) && WF_COMBAT_TAG_STARTUP_LZ4
 wf_tag_start_scene_fields=wf_fields-tag_scene_started;
#endif
#endif
 /* HW1's immutable warm list changes transport residency only. Its native
  * loader completes each empty VDP1 batch while the display is hidden;
  * it never advances the actors, RNG, render queue or logical clock. */
#ifndef WF_COMBAT_TAG_TRANSPORT
 if(s->runtime.g.mode_0161&1){
  if(file_size("SPRWARM.BIN",&size)||wf_r18_present_prewarm("SPRWARM.BIN",size)!=WF_OK)goto failed;
  wf_combat_warm_tiles=(size-8u)/2u;
 }
#endif
 wf_combat_stage=3;
#ifndef WF_COMBAT_BASE_ROYAL
 if(!map||file_size(map,&size)||size>WF_SATURN_LWRAM_SCENE_BYTES||file_read(map,0,scene,size)||
  (((unsigned)scene[6]<<8)|scene[7])!=world||wf_r18_ring_bind(scene,size))goto failed;
#endif
#if defined(WF_COMBAT_TAG_TRANSPORT) || defined(WF_COMBAT_HUD_WINDOW)
 if(hud_load(&s->video)!=WF_OK)goto failed;
#else
 if(wf_r18_hud_bind(&s->video,file_read)!=WF_OK)goto failed;
#endif
 wf_r18_video_ring_bind(&s->video);wf_r18_hud_reset(&s->video);
 if(wf_r18_display_init(&display,&s->runtime,wf_r18_rom_read,NULL)!=WF_OK)goto failed;
 transition=(wf_r18_transition_context_t){.video=&s->video,.world=&combat.world,.display=&display,
  .camera=&combat.gameplay.camera,.render_queue=&combat.gameplay.render_queue,
  .scene_blob=scene,.scene_capacity=WF_SATURN_LWRAM_SCENE_BYTES,.file_size=file_size,.file_read=file_read,
  .priority_140010=&priority,.unblank_pending=1};
 wf_r18_transition_bind(&transition);
#ifdef WF_COMBAT_TAG_TRANSPORT
 void*tail=NULL;
 if(file_size(WP2015_SECONDARY_FILE,&size)||wf_r18_present_secondary_lease(WP2015_SECONDARY_LWRAM_BYTES,&tail)!=WF_OK||
    secondary_load(tail,size))goto failed;
#endif
 if(!palettes_ring())goto failed;
#ifdef WF_COMBAT_BANKED_TRANSPORT
 vdp2_scrn_display_set(0);vdp2_sync();vdp2_sync_wait();
#ifdef WF_COMBAT_HW1_BANKED
 if(wf_saturn_yaul_display_begin()!=WF_OK)goto failed;
#else
 if(wp2015_display_begin(1u))goto failed;
#endif
#endif
 wf_r18_cdfs_combat_window_set(1);
 wf_combat_stage=4;wf_combat_rng=s->runtime.g.rng_state;
#ifdef WF_COMBAT_ROYAL_FULL_CROP
 royal_full_observe();
#endif
 return wf_platform_load(false);
#if defined(WF_COMBAT_TAG_ATLAS_ARCHIVE) && WF_COMBAT_TAG_ATLAS_ARCHIVE
 failed:wf_combat_failure_site=0x10u+wf_combat_stage;wf_combat_error=wf_combat_stage;
 if(tag_preload_route)tag_preload_fail(wf_combat_tag_error?wf_combat_tag_error:7);
#ifdef WF_COMBAT_ROYAL_FULL_CROP
 royal_full_observe();wf_r18_full_crop_reset();
#endif
 wf_platform_load(false);return false;
#else
 failed:wf_combat_failure_site=0x10u+wf_combat_stage;wf_combat_error=wf_combat_stage;
#ifdef WF_COMBAT_ROYAL_FULL_CROP
 royal_full_observe();wf_r18_full_crop_reset();
#endif
 wf_platform_load(false);return false;
#endif
}
#ifdef WF_ALPHA_RETURN
__attribute__((cold,noinline)) bool wf_combat_saturn_export_scene(void){
 if(!session||!board||!wf_combat_finished)return false;
 if(wf_scene_background_capture(&session->video)!=WF_OK)return false;
 size_t capacity=0;
 const wf_r18_sprite_record_t *records=wf_r18_present_records_buffer(&capacity);
 size_t count=wf_r18_present_records_count();
 if(count>capacity||wf_scene_checkpoint_capture(&session->runtime,session->ram,sizeof session->ram,
    &session->video,&combat.world,records,count,
    combat.gameplay.render_queue.count)!=WF_OK)return false;
 for(unsigned i=0;i<4;i++)if(!wf_board_write(board,0x100000u+i*2u,display.scroll[i],2))return false;
 if(!wf_board_write(board,0x140010u,priority,2))return false;
 session->frontend.regs=session->runtime.m68k_regs;session->frontend.world_visible=1;
#ifdef WF_COMBAT_ROYAL_FULL_CROP
 wf_r18_full_crop_reset();royal_full_mode=false;
#endif
 return true;
}
#endif
#ifdef WF_RESULT_PRELOAD
#include "wf_result_saturn.inc"
#endif
bool wf_combat_saturn_step(const uint8_t inputs[4]){
 if(wf_combat_stage<4){wf_combat_failure_site=9;return false;}
 wf_runtime_t*r=&session->runtime;
 for(unsigned i=0;i<4;i++)combat.gameplay.input_hw_low[i]=inputs[i];
 combat.gameplay.cabinet_word_140020=(uint16_t)(0xff00|inputs[0]);combat.gameplay.cabinet_word_140022=(uint16_t)(0xff00|inputs[1]);
 uint32_t profile[8],fields=wf_fields,sectors=wf_cd_sectors;
 unsigned begin=cpu_frt_count_get(),at=begin,now,work,wait;
 wf_status_t st=wf_runtime_frame(r);
 now=cpu_frt_count_get();profile[0]=(uint16_t)(now-at);at=now;
 if(st!=WF_BLOCKED||!r->frame_ready){wf_combat_failure_site=1;goto failed;}
 if(wf_r18_display_irq(r,&display)!=WF_OK){wf_combat_failure_site=2;goto failed;}
 if(wf_r18_world_commit(r,&combat.world)!=WF_OK){wf_combat_failure_site=3;goto failed;}
 if(wf_r18_hud_irq(r,&session->video)!=WF_OK){wf_combat_failure_site=4;goto failed;}
 if(!palettes_ring()){wf_combat_failure_site=5;goto failed;}
 now=cpu_frt_count_get();profile[1]=(uint16_t)(now-at);work=(uint16_t)(now-begin);at=begin=now;
#ifdef WF_COMBAT_BANKED_TRANSPORT
 if(wf_saturn_yaul_frame_sync()!=WF_OK){wf_combat_failure_site=6;goto failed;}
 now=cpu_frt_count_get();profile[2]=(uint16_t)(now-at);at=now;
 wf_saturn_yaul_frame_wait();
 now=cpu_frt_count_get();profile[3]=(uint16_t)(now-at);profile[4]=0;wait=(uint16_t)(now-begin);at=now;
 if(wp2015_display_observer[8]){wf_combat_failure_site=7;goto failed;}
#else
 vdp1_sync();vdp2_sync();
 now=cpu_frt_count_get();profile[2]=(uint16_t)(now-at);at=now;
 vdp1_sync_wait();
 now=cpu_frt_count_get();profile[3]=(uint16_t)(now-at);at=now;
 vdp2_sync_wait();
 now=cpu_frt_count_get();profile[4]=(uint16_t)(now-at);wait=(uint16_t)(now-begin);at=now;
#endif
 if(transition.unblank_pending){
#ifdef WF_COMBAT_BANKED_TRANSPORT
  wf_saturn_yaul_display_drain();
#endif
  vdp2_scrn_display_set(VDP2_SCRN_DISPTP_NBG2|(priority==0x78?(VDP2_SCRN_DISPTP_NBG0|VDP2_SCRN_DISP_NBG1):(VDP2_SCRN_DISP_NBG0|VDP2_SCRN_DISPTP_NBG1)));
  vdp2_sprite_priority_set(0,priority==0x7c?2:3);vdp2_tvmd_display_set();transition.unblank_pending=0;
#if defined(WF_TAG_PREFETCH) && WF_TAG_PREFETCH
  wf_tag_prefetch_cart_unblank();
#endif
 }
 ++wf_combat_profile_sequence;
 wf_runtime_vblank_ack(r);++wf_combat_ack;st=wf_runtime_frame(r);
#ifdef WF_COMBAT_ROYAL_FULL_CROP
 royal_full_observe();
#endif
 wf_combat_frame=r->g.frame_counter;wf_combat_rng=r->g.rng_state;wf_combat_sprites=wf_r18_present_commands();wf_combat_stage=5;
 profile[5]=(uint16_t)(cpu_frt_count_get()-at);profile[6]=wf_cd_sectors-sectors;profile[7]=wf_fields-fields;
 wf_combat_work=work;wf_combat_wait=wait;
 for(unsigned i=0;i<8;i++){wf_combat_profile[i]=profile[i];wf_combat_profile_total[i]+=profile[i];}
 ++wf_combat_profile_sequence;
 if(st==WF_OK)return true;
#ifdef WF_COMBAT_MODULE
 /* The recovered runners distinguish a completed match from a missing
  * gameplay owner at the post-ACK boundary. Keep its final image while
  * the alpha exports it before resuming the original presentation owners. */
 if(st==WF_BLOCKED&&(wf_r18_tag_match_complete(r)||wf_r18_royal_game_over(r)||wf_r18_attract_timeout(r)
#ifdef WF_ALPHA_RETURN
 ||wf_royal_finale_ready(r)
#endif
 )){
#ifdef WF_COMBAT_BANKED_TRANSPORT
  wf_saturn_yaul_display_drain();wp2015_display_stop();
#endif
  #ifdef WF_RESULT_PRELOAD
  ++wf_result_sequence;wf_result_terminal_cd=wf_cd_sectors;wf_result_terminal_field=wf_fields;wf_result_status=1;wf_result_error=0;++wf_result_sequence;
#endif
  wf_combat_finished=1;return true;
 }
#endif
 wf_combat_failure_site=8;
 failed:wf_combat_error=combat.gameplay.last_blocked_phase;wf_combat_dependency=combat.gameplay.last_blocked_dependency;
#ifdef WF_COMBAT_ROYAL_FULL_CROP
 if(wf_combat_full_error)wf_combat_dependency=0xa600u|wf_combat_full_error;
#endif
#ifdef WF_COMBAT_GLOBAL_RANS
 if(wf_combat_global_error)wf_combat_dependency=0xa400u|wf_combat_global_error;
#endif
#if defined(WF_COMBAT_TAG_ATLAS_ARCHIVE) && WF_COMBAT_TAG_ATLAS_ARCHIVE
 if(wf_combat_tag_error)wf_combat_dependency=0xa500u|wf_combat_tag_error;
#endif
 return false;
}
