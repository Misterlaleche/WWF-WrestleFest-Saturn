#include <string.h>
#include "wf_saturn_yaul_backend.h"

#ifdef WF_PLATFORM_SATURN
#include <yaul.h>
static vdp1_cmdt_t g_cmdts[WF_VDP1_COMMANDS_MAX+4u] __aligned(32);
static unsigned g_cmd_index;
#endif

static wf_status_t begin_frame(void *ctx,unsigned nt,unsigned np,unsigned nc){
    wf_saturn_yaul_backend_t*b=(wf_saturn_yaul_backend_t*)ctx;
    if(!b||b->active)return WF_BAD_STATE;
    if(nt>WF_VDP1_STREAM_TILES_MAX||np>WF_SATURN_CRAM_SPRITE_BANKS||nc>WF_VDP1_COMMANDS_MAX)return WF_BLOCKED;
    if(nt>WF_SATURN_VDP1_STREAM_TILE_CAPACITY||nc+4u+1u>WF_SATURN_VDP1_CMD_COUNT)return WF_BLOCKED;
#ifdef WF_PLATFORM_SATURN
    if(nt&&(!b->hwr_stage||b->hwr_stage_bytes<(size_t)nt*WF_VDP1_TILE_BYTES_16X16_4BPP))return WF_BLOCKED;
#endif
    b->expected_tiles=nt;b->expected_palettes=np;b->expected_cmds=nc;
    b->emitted_tiles=b->emitted_palettes=b->emitted_cmds=0;b->active=1;
    memset(b->dirty,0,sizeof b->dirty);b->palette_mask=0;
#ifdef WF_PLATFORM_SATURN
    memset(g_cmdts,0,sizeof(g_cmdts));g_cmd_index=0;
    vdp1_cmdt_system_clip_coord_set(&g_cmdts[g_cmd_index]);vdp1_cmdt_vtx_system_clip_coord_set(&g_cmdts[g_cmd_index++],INT16_VEC2(319,239));
    vdp1_cmdt_local_coord_set(&g_cmdts[g_cmd_index]);vdp1_cmdt_vtx_local_coord_set(&g_cmdts[g_cmd_index++],INT16_VEC2(0,0));
    /* VDP1's transparent erase exposes the lower-priority VDP2 ring.
     * An opaque clearing polygon would cover both bitmap layers. */
#endif
    return WF_OK;
}

static wf_status_t upload_tile(void *ctx,uint16_t slot,const uint8_t*src,size_t bytes){
    wf_saturn_yaul_backend_t*b=(wf_saturn_yaul_backend_t*)ctx;uint32_t off;
    if(!b||!b->active||!src||bytes!=WF_VDP1_TILE_BYTES_16X16_4BPP)return WF_BAD_STATE;
    if(slot!=b->emitted_tiles||b->emitted_tiles>=b->expected_tiles)return WF_BAD_STATE;
    if(wf_saturn_stream_tile_offset(slot,&off)!=WF_OK)return WF_BLOCKED;
#ifdef WF_PLATFORM_SATURN
    (void)off;
    /* Stage the complete contiguous stream in HWRAM: SCU-DMA cannot read
     * LWRAM. No VRAM transfer occurs until the frame is complete. */
    unsigned mask=1u<<(slot&7u);uint8_t *dst=b->hwr_stage+(size_t)slot*bytes;
    if((b->resident[slot>>3]&mask)&&!memcmp(dst,src,bytes))++b->texture_reused;
    else {
        memcpy(dst,src,bytes);b->dirty[slot>>3]|=(uint8_t)mask;
        /* Staging no longer describes VRAM until end_frame commits it. */
        b->resident[slot>>3]&=(uint8_t)~mask;
    }
#else
    (void)off;(void)bytes;
#endif
    b->rom_identity[slot>>3]&=(uint8_t)~(1u<<(slot&7u));
    b->emitted_tiles++;return WF_OK;
}

wf_status_t wf_saturn_yaul_upload_rom_tile(wf_saturn_yaul_backend_t*b,uint16_t slot,uint16_t source,wf_status_t (*read_tile)(uint16_t,const uint8_t**)){
 if(!b||!b->active||!read_tile||slot!=b->emitted_tiles||slot>=b->expected_tiles)return WF_BAD_STATE;
 unsigned mask=1u<<(slot&7u);
 if((b->resident[slot>>3]&mask)&&(b->rom_identity[slot>>3]&mask)&&b->rom_source[slot]==source){
  ++b->emitted_tiles;++b->texture_reused;return WF_OK;
 }
 const uint8_t *p;wf_status_t s=read_tile(source,&p);if(s!=WF_OK)return s;
 s=upload_tile(b,slot,p,128);
 if(s==WF_OK){b->rom_source[slot]=source;b->rom_identity[slot>>3]|=(uint8_t)mask;}
 return s;
}

static wf_status_t upload_palette(void *ctx,uint8_t bank,const uint8_t*src,size_t bytes){
    wf_saturn_yaul_backend_t*b=(wf_saturn_yaul_backend_t*)ctx;
    if(!b||!b->active||!src||bytes!=WF_VDP1_PALETTE_BYTES||bank>=WF_SATURN_CRAM_SPRITE_BANKS)return WF_BAD_STATE;
    if(b->emitted_palettes>=b->expected_palettes||(b->palette_mask&(1u<<bank)))return WF_BAD_STATE;
    memcpy(b->palette_stage[bank],src,32);b->palette_mask|=(uint16_t)(1u<<bank);
    b->emitted_palettes++;return WF_OK;
}

static wf_status_t emit_sprite(void *ctx,const wf_vdp1_sprite_cmd_t*c){
    wf_saturn_yaul_backend_t*b=(wf_saturn_yaul_backend_t*)ctx;uint32_t off;
    if(!b||!b->active||!c)return WF_BAD_STATE;
    if(c->cram_slot>=WF_SATURN_CRAM_SPRITE_BANKS)return WF_BLOCKED;
    if(wf_saturn_stream_tile_offset(c->stream_tile,&off)!=WF_OK)return WF_BLOCKED;
#ifdef WF_PLATFORM_SATURN
    if(g_cmd_index>=WF_VDP1_COMMANDS_MAX+3u)return WF_BLOCKED;
    vdp1_cmdt_t*cmd=&g_cmdts[g_cmd_index++];memset(cmd,0,sizeof(*cmd));
    vdp1_cmdt_normal_sprite_set(cmd);
    /* Arcade pen F is a color, not VDP1's end-of-line marker. */
    vdp1_cmdt_draw_mode_t dm={.raw=0};dm.color_mode=VDP1_CMDT_CM_CB_16;dm.cc_mode=VDP1_CMDT_CC_REPLACE;dm.trans_pixel_disable=0;dm.end_code_disable=1;
    vdp1_cmdt_draw_mode_set(cmd,dm);
    vdp1_cmdt_char_base_set(cmd,VDP1_VRAM(off));
    vdp1_cmdt_char_size_set(cmd,16,16);
    vdp1_cmdt_char_flip_t f=VDP1_CMDT_CHAR_FLIP_NONE;if(c->flip_x)f=(vdp1_cmdt_char_flip_t)(f|VDP1_CMDT_CHAR_FLIP_H);if(c->flip_y)f=(vdp1_cmdt_char_flip_t)(f|VDP1_CMDT_CHAR_FLIP_V);vdp1_cmdt_char_flip_set(cmd,f);
    vdp1_cmdt_color_bank_t cb={.raw=(uint16_t)((WF_SATURN_CRAM_SPRITE_FIRST+(unsigned)c->cram_slot*16u)&0xFFF0u)};vdp1_cmdt_color_mode0_set(cmd,cb);
    int16_vec2_t pos={.x=c->x,.y=c->y};vdp1_cmdt_vtx_zoom_point_set(cmd,pos);
#else
    (void)off;
#endif
    b->emitted_cmds++;return WF_OK;
}

static wf_status_t end_frame(void *ctx){
    wf_saturn_yaul_backend_t*b=(wf_saturn_yaul_backend_t*)ctx;
    if(!b||!b->active)return WF_BAD_STATE;
    if(b->emitted_tiles!=b->expected_tiles||b->emitted_palettes!=b->expected_palettes||b->emitted_cmds!=b->expected_cmds){b->active=0;return WF_BAD_STATE;}
#ifdef WF_PLATFORM_SATURN
    if(g_cmd_index>=WF_VDP1_COMMANDS_MAX+4u){b->active=0;return WF_BLOCKED;}
    /* Each DMA contains only a consecutive run whose resident bytes changed. */
    for(unsigned slot=0;slot<b->expected_tiles;){
        if(!(b->dirty[slot>>3]&(1u<<(slot&7u)))){++slot;continue;}
        unsigned first=slot++;
        while(slot<b->expected_tiles&&(b->dirty[slot>>3]&(1u<<(slot&7u))))++slot;
        size_t bytes=(size_t)(slot-first)*128u;
        scu_dma_transfer(0,(void*)VDP1_VRAM(WF_SATURN_VDP1_DYNAMIC_OFFSET+first*128u),b->hwr_stage+first*128u,bytes);
        scu_dma_transfer_wait(0);b->texture_bytes+=(uint32_t)bytes;++b->texture_transfers;
        for(unsigned i=first;i<slot;i++)b->resident[i>>3]|=(uint8_t)(1u<<(i&7u));
    }
    for(unsigned bank=0;bank<16;bank++)if(b->palette_mask&(1u<<bank))
        wf_saturn_cram_write_bank((uint16_t)(WF_SATURN_CRAM_SPRITE_FIRST+bank*16u),b->palette_stage[bank]);
    memset(&g_cmdts[g_cmd_index],0,sizeof(g_cmdts[g_cmd_index]));vdp1_cmdt_end_set(&g_cmdts[g_cmd_index]);g_cmd_index++;
    vdp1_cmdt_list_t list;vdp1_cmdt_list_init(&list,g_cmdts);list.count=(uint16_t)g_cmd_index;
    vdp1_sync_cmdt_list_put(&list,0);vdp1_sync_put_wait();vdp1_sync_render();
    if(!b->defer_sync){vdp1_sync();vdp1_sync_wait();}
#endif
    b->active=0;return WF_OK;
}


static void abort_frame(void *ctx){
    wf_saturn_yaul_backend_t*b=(wf_saturn_yaul_backend_t*)ctx;
    if(!b)return;
    b->active=0;
    b->expected_tiles=b->expected_palettes=b->expected_cmds=0;
    b->emitted_tiles=b->emitted_palettes=b->emitted_cmds=0;
#ifdef WF_PLATFORM_SATURN
    g_cmd_index=0;
    memset(g_cmdts,0,sizeof(g_cmdts));
#endif
}

static const wf_saturn_render_ops_t g_ops={begin_frame,upload_tile,upload_palette,emit_sprite,end_frame,abort_frame};
void wf_saturn_yaul_backend_init(wf_saturn_yaul_backend_t*b,void*h,size_t n){if(!b)return;memset(b,0,sizeof(*b));b->hwr_stage=(uint8_t*)h;b->hwr_stage_bytes=n;}
const wf_saturn_render_ops_t*wf_saturn_yaul_render_ops(void){return &g_ops;}
wf_status_t wf_saturn_yaul_present_plan(wf_saturn_yaul_backend_t*b,const wf_vdp1_frame_plan_t*p,const wf_vdp1_assets_t*a){return wf_saturn_render_submit(p,a,&g_ops,b);}

wf_status_t wf_saturn_yaul_video_init(void){
    if(wf_saturn_memory_contract_validate()!=WF_OK)return WF_BAD_STATE;
#ifdef WF_PLATFORM_SATURN
    wf_saturn_cram_reset();
    vdp2_cram_mode_set(WF_SATURN_CRAM_MODE);
    vdp1_vram_partitions_set(WF_SATURN_VDP1_CMD_COUNT,0x53560u,0,0);
    vdp1_env_t env;vdp1_env_default_init(&env);env.bpp=VDP1_ENV_BPP_16;env.color_mode=VDP1_ENV_COLOR_MODE_PALETTE;env.sprite_type=0;env.erase_color=RGB1555(0,0,0,0);env.erase_points[0]=(int16_vec2_t){.x=0,.y=0};env.erase_points[1]=(int16_vec2_t){.x=319,.y=239};vdp1_env_set(&env);
    vdp1_sync_interval_set(0);vdp1_sync_mode_set(VDP1_SYNC_MODE_ERASE_CHANGE);
    vdp2_sprite_priority_set(0,3);
#endif
    return WF_OK;
}
