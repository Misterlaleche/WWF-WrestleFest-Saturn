#include <yaul.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include "wf_intro_presentation.h"
#ifndef WF_HARNESS_FBI_ENABLED
#define WF_HARNESS_FBI_ENABLED 1
#endif
#define INTRO_CRAM_BASE VDP2_CRAM_MODE_1_OFFSET(0,0,0)
#define INTRO_PAGE0 VDP2_VRAM_ADDR(0,0)
#define INTRO_PAGE1 VDP2_VRAM_ADDR(1,0)
#define INTRO_PAGE2 VDP2_VRAM_ADDR(2,0)
extern uint8_t asset_intro[];
static uint8_t __aligned(32) staging[WF_INTRO_VRAM_PAGE_BYTES];
static uint8_t __aligned(32) palette_work[WF_INTRO_PALETTE_BYTES];
static const vdp2_vram_t scene_pages[WF_INTRO_SCENE_COUNT]={INTRO_PAGE0,INTRO_PAGE1,INTRO_PAGE2};
static void dma_copy(void *dst,const void *src,size_t size){scu_dma_transfer(0,dst,src,size);scu_dma_transfer_wait(0);}
static void preload_scene(unsigned scene){
    const wf_intro_asset_t *a=&wf_intro_assets[scene];
    memset(staging,0,sizeof(staging));
    if(!wf_intro_unpack(asset_intro+a->offset,a->size,staging,WF_INTRO_BITMAP_BYTES)) while(true){}
    if(wf_intro_fnv1a(staging,WF_INTRO_BITMAP_BYTES)!=a->bitmap_fnv1a) while(true){}
    dma_copy((void *)scene_pages[scene],staging,sizeof(staging));
}
static void wait_vblank(void){vdp2_sync();vdp2_sync_wait();}
static void commit_palette(const uint8_t *p){vdp_dma_enqueue((void *)INTRO_CRAM_BASE,p,WF_INTRO_PALETTE_BYTES);wait_vblank();}
static void set_scene_bitmap(unsigned scene){
    const vdp2_scrn_bitmap_format_t f={
        .scroll_screen=VDP2_SCRN_NBG0,.ccc=VDP2_SCRN_CCC_PALETTE_256,
        .bitmap_size=VDP2_SCRN_BITMAP_SIZE_512X256,.palette_base=INTRO_CRAM_BASE,
        .bitmap_base=scene_pages[scene]
    };
    vdp2_scrn_bitmap_format_set(&f);vdp2_scrn_scroll_x_set(VDP2_SCRN_NBG0,0);vdp2_scrn_scroll_y_set(VDP2_SCRN_NBG0,0);
}
static void publish_scene(unsigned scene){
    const wf_intro_asset_t *a=&wf_intro_assets[scene];
    memcpy(palette_work,asset_intro+a->palette_offset,WF_INTRO_PALETTE_BYTES);
    set_scene_bitmap(scene);commit_palette(palette_work);
}
static void apply_fade(unsigned scene,unsigned level){
    const wf_intro_asset_t *a=&wf_intro_assets[scene];
    wf_intro_build_fade_palette(asset_intro+a->palette_offset,level,palette_work);commit_palette(palette_work);
}
void user_init(void){
    vdp2_tvmd_display_clear();
    vdp2_tvmd_display_res_set(VDP2_TVMD_INTERLACE_NONE,VDP2_TVMD_HORZ_NORMAL_A,VDP2_TVMD_VERT_240);
    vdp2_cram_mode_set(1);vdp2_scrn_back_color_set(VDP2_VRAM_ADDR(3,0x1FFFE),RGB1555(1,0,0,0));
    vdp2_scrn_priority_set(VDP2_SCRN_NBG0,3);
    vdp2_vram_cycp_t cycles=*vdp2_vram_cycp_get();
    for(unsigned bank=0;bank<3U;bank++){
        cycles.pt[bank].t0=VDP2_VRAM_CYCP_CHPNDR_NBG0;cycles.pt[bank].t1=VDP2_VRAM_CYCP_CHPNDR_NBG0;
        cycles.pt[bank].t2=VDP2_VRAM_CYCP_CHPNDR_NBG0;cycles.pt[bank].t3=VDP2_VRAM_CYCP_CHPNDR_NBG0;
        cycles.pt[bank].t4=VDP2_VRAM_CYCP_NO_ACCESS;cycles.pt[bank].t5=VDP2_VRAM_CYCP_NO_ACCESS;
        cycles.pt[bank].t6=VDP2_VRAM_CYCP_CPU_RW;cycles.pt[bank].t7=VDP2_VRAM_CYCP_CPU_RW;
    }
    vdp2_vram_cycp_set(&cycles);vdp2_sync();vdp2_sync_wait();
}
int main(void){
    if(wf_intro_fnv1a(asset_intro,WF_INTRO_ASSET_BYTES)!=0x69055E2BU) while(true){}
    for(unsigned scene=0;scene<WF_INTRO_SCENE_COUNT;scene++) preload_scene(scene);
    wf_intro_state_t state;wf_intro_event_t e=wf_intro_init(&state,WF_HARNESS_FBI_ENABLED!=0);
    publish_scene((unsigned)e.scene);vdp2_scrn_display_set(VDP2_SCRN_DISP_NBG0);vdp2_tvmd_display_set();
    while(state.phase!=WF_INTRO_PHASE_HANDOFF){
        e=wf_intro_vblank(&state);
        switch(e.type){
        case WF_INTRO_EVENT_NONE:wait_vblank();break;
        case WF_INTRO_EVENT_APPLY_FADE:apply_fade((unsigned)e.scene,e.fade_level);break;
        case WF_INTRO_EVENT_PUBLISH_SCENE:publish_scene((unsigned)e.scene);break;
        case WF_INTRO_EVENT_FAULT:vdp2_tvmd_display_clear();while(true)wait_vblank();
        case WF_INTRO_EVENT_TITLE_HANDOFF:break;
        default:vdp2_tvmd_display_clear();while(true)wait_vblank();
        }
    }
    while(true)wait_vblank();
}
