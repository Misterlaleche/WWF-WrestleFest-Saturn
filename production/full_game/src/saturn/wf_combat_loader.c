#include "wf_combat_loader.h"
#include "wf_platform.h"
#if !defined(WF_COMBAT_MODULE)
#include "wf_live_video.h"
#endif
#include "wf_cd.h"
#include "wf_combat_image.h"
#ifdef WF_ALPHA_RETURN
#include "wf_overlay_irq.h"
#endif
#include "wf_r18_cdfs.h"
#if defined(WF_CD_RATE_GUARDED) || defined(WF_COMBAT_TAG_TRANSPORT)
#include "wp2015_cd_rate.h"
#endif
#include <string.h>
extern const uint8_t wf_combat_jump[],wf_combat_jump_end[];
extern volatile uint32_t wf_clock_origin;
_Static_assert(sizeof(wf_combat_handoff_t)<=0x23000u,"capsule below module staging");
#if defined(WF_COMBAT_MODULE)
#if !defined(WF_ALPHA_RETURN)
#error A combat loader is only linked for the presentation alpha return path
#endif
__attribute__((cold,noinline)) bool wf_combat_return_frontend(wf_session_t*s,wf_board_t*b,uint32_t cold_rng){
#else
bool wf_combat_load_module(wf_session_t*s,wf_board_t*b,uint32_t cold_rng){
#endif
 if(!s||!b||!wf_platform_load(true))return false;
#ifdef WF_COMBAT_MODULE
 const char *image="FRONT.BIN";
 wf_r18_cdfs_combat_window_set(0);WF_COMBAT_MAILBOX[0]=4;
#else
 const char *image="COMBAT.BIN",*manifest="COMBAT.HDR";
#ifdef WF_COMBAT_VARIANTS
#if defined(WF_TAG_CART_PRELOAD) && WF_TAG_CART_PRELOAD
 if(!(s->runtime.g.mode_0161&1u)&&wf_cart_valid&&wf_cart_bytes==0x400000u){image="CART.BIN";manifest="CART.HDR";}
 else
#endif
 if(!(s->runtime.g.mode_0161&1u)){image="TAG.BIN";manifest="TAG.HDR";}
 else if(wf_cart_valid&&wf_cart_bytes==0x400000u){image="CART.BIN";manifest="CART.HDR";}
 else {image="ROYAL.BIN";manifest="ROYAL.HDR";}
#endif
 wf_live_video_finish();WF_COMBAT_MAILBOX[0]=1;
#endif
 uint8_t header[32];uint32_t bytes=0;wf_pacer_t saved;
 if(wf_cd_asset_io.size(NULL,image,&bytes))goto failed;
#ifdef WF_COMBAT_MODULE
 /* The return image keeps header and payload in one root entry. Its first
  * sector contains the manifest; the executable starts sector-aligned. */
 if(bytes<2048u)goto failed;
 bytes-=2048u;
 if(wf_cd_asset_io.read(NULL,image,0,header,sizeof header))goto failed;
 const uint32_t payload_offset=2048u;
#else
 if(wf_cd_asset_io.read(NULL,manifest,0,header,sizeof header))goto failed;
 const uint32_t payload_offset=0;
#endif
 if(bytes<8192u||bytes>WF_COMBAT_MODULE_LIMIT||(bytes&3u)||
    !wf_combat_image_header(header,bytes)||
    wf_cd_asset_io.read(NULL,image,payload_offset,(void*)WF_COMBAT_MODULE_STAGE,bytes))goto failed;
 uint32_t main_bytes=wf_combat_be32(header+16),tail_at=wf_combat_be32(header+20),tail_bytes=wf_combat_be32(header+24);
 uint32_t hash=wf_combat_image_hash((const uint8_t*)WF_COMBAT_MODULE_STAGE,bytes);
 if(hash!=wf_combat_be32(header+12))goto failed;
#if defined(WF_CD_RATE_GUARDED) || defined(WF_COMBAT_TAG_TRANSPORT)
 /* The destination CRT resets FRT and drops this command history. Drain the
  * last CD epoch before masking interrupts or transferring ownership. */
 int rate_rc=wp2015_cd_rate_before_frt_reset();
 if(rate_rc){wf_cd_error=(uint32_t)rate_rc;goto failed;}
#endif
 if(!wf_platform_clock_export(&saved))goto failed;
#ifdef WF_COMBAT_MODULE
 if(!wf_combat_handoff_pack_return(WF_COMBAT_CAPSULE,s,b,&saved))goto failed;
#else
 if(!wf_combat_handoff_pack(WF_COMBAT_CAPSULE,s,b,&saved))goto failed;
#endif
 WF_COMBAT_CAPSULE->clock_origin=wf_clock_origin;WF_COMBAT_CAPSULE->cold_rng=cold_rng;
 wf_combat_handoff_seal(WF_COMBAT_CAPSULE);
 size_t jump_bytes=(size_t)(wf_combat_jump_end-wf_combat_jump);
 if(!jump_bytes||jump_bytes>0x800u)goto failed;
 memcpy((void*)0x20200000u,wf_combat_jump,jump_bytes);
 /* CD and video transfers finish while their IRQ owners still exist. */
 scu_dma_transfer_wait(0);scu_dma_transfer_wait(1);scu_dma_transfer_wait(2);
 cpu_dmac_channel_wait(0);cpu_dmac_channel_wait(1);
 WF_COMBAT_MAILBOX[1]=WF_COMBAT_CAPSULE->checksum;WF_COMBAT_MAILBOX[2]=hash;
 cpu_intc_mask_set(15);scu_ic_mask_set(SCU_IC_MASK_ALL);
#ifdef WF_ALPHA_RETURN
 wf_overlay_irq_clear();
#endif
 cpu_cache_disable();
 WF_COMBAT_MAILBOX[0]=
#ifdef WF_COMBAT_MODULE
 5;
#else
 2;
#endif
 ((void(*)(const void*,uint32_t,void*,uint32_t))0x20200000u)((const void*)WF_COMBAT_MODULE_STAGE,main_bytes/4u,(void*)(tail_at|0x20000000u),tail_bytes/4u);
 __builtin_unreachable();
 failed:WF_COMBAT_MAILBOX[0]=
#ifdef WF_COMBAT_MODULE
 0xdead0004u;
#else
 0xdead0001u;
#endif
 return false;
}
