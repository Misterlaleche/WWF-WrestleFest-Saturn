#include <string.h>
#include "wf_saturn_yaul_backend.h"
#include "wp1980_vram_cache.h"

/* SH-2 has no variable barrel shift. These byte-sized masks avoid a
 * software shift call at each tile/residency/dirty test in the hot path. */
static unsigned byte_mask(unsigned bit){
    static const uint8_t masks[8]={1,2,4,8,16,32,64,128};
    return masks[bit&7u];
}

#ifdef WF_PLATFORM_SATURN
#include <yaul.h>
static vdp1_cmdt_t g_cmdts[WF_VDP1_COMMANDS_MAX+4u] __aligned(32);
static unsigned g_cmd_index;
#define WP1980_VRAM_POOL_OFFSET 0x08020u
#define WP1980_VRAM_POOL_END (WP1980_VRAM_POOL_OFFSET+WP1980_VRAM_CACHE_SLOTS*128u)
_Static_assert(WP1980_VRAM_POOL_OFFSET>=WF_SATURN_VDP1_CMD_OFFSET+WF_SATURN_VDP1_CMD_BYTES,"ROM pool follows command tables");
_Static_assert(WP1980_VRAM_POOL_END<=WF_SATURN_VDP1_VRAM_BYTES,"ROM pool fits physical VDP1 VRAM");
#if WP1980_VRAM_POOL_END>WF_SATURN_VDP1_DYNAMIC_OFFSET
/* Expanded lease occupies otherwise idle legacy/music VRAM. It is legal
 * only when the build suppresses MUSIC.ELF and slave music execution. A
 * generic stream invalidates the entire ROM cache before overlapping DMA. */
#if !defined(WP1980_DISABLE_MUSIC) || !WP1980_DISABLE_MUSIC
#error Expanded ROM texture pool requires WP1980_DISABLE_MUSIC=1
#endif
#endif
static wp1980_vram_cache_t g_rom_cache;
static wf_saturn_yaul_backend_t *g_rom_owner,*g_rom_prepared;
volatile uint32_t wp1980_vram_hits,wp1980_vram_misses,wp1980_vram_evictions;


static unsigned row_nonzero(const uint8_t *row){
    /* Freestanding SH-2 builds keep memcpy as an out-of-line call. Eight
     * byte reads also accept unaligned caller-owned staging buffers. */
    return (unsigned)row[0]|row[1]|row[2]|row[3]|row[4]|row[5]|row[6]|row[7];
}

enum { CROP_TOP_MASK=0x1fu,CROP_NARROW=0x20u,CROP_RIGHT=0x40u };
_Static_assert(WF_VDP1_TILE_BYTES_16X16_4BPP==128u,"Cropping owns one 16x16 4bpp tile");
_Static_assert(WF_SATURN_VDP1_STREAM_TILE_BYTES==128u,"VRAM slots retain their 128-byte stride");

static unsigned half_nonzero(const uint8_t *row){
    return (unsigned)row[0]|row[1]|row[2]|row[3];
}

static void cache_crop(wf_saturn_yaul_backend_t *b,unsigned slot,uint8_t *tile){
    unsigned top=0,bottom=16;
    while(top<16u&&!row_nonzero(tile+top*8u))++top;
    while(bottom>top&&!row_nonzero(tile+(bottom-1u)*8u))--bottom;
    unsigned meta=top;
    if(top<bottom){
        unsigned left=0,right=0;
        for(unsigned y=top;y<bottom&&!(left&&right);++y){
            left|=half_nonzero(tile+y*8u);
            right|=half_nonzero(tile+y*8u+4u);
        }
        if(!left||!right){
            unsigned column=left?0u:4u;
            meta|=CROP_NARROW;if(column)meta|=CROP_RIGHT;
            /* Eight-pixel rows occupy four bytes. Pack the cropped image
             * at the slot base so odd crop_top values also have an 8-byte
             * aligned SRCA. Forward copies are safe: source>=destination. */
            uint8_t *dst=tile;
            const uint8_t *src=tile+top*8u+column;
            for(unsigned y=top;y<bottom;++y){
                dst[0]=src[0];dst[1]=src[1];dst[2]=src[2];dst[3]=src[3];
                dst+=4u;src+=8u;
            }
        }
    }
    b->crop_meta[slot]=(uint8_t)meta;b->crop_height[slot]=(uint8_t)(bottom-top);
}

static unsigned tile_matches(wf_saturn_yaul_backend_t *b,unsigned slot,const uint8_t *src,const uint8_t *dst){
    unsigned meta=b->crop_meta[slot];
    if(!(meta&CROP_NARROW))return !memcmp(dst,src,128u);
    /* Staging is compacted, while a generic caller supplies a raw16x16
     * tile. Compare its full logical picture, including the discarded
     * transparent rows/half. A raw memcmp here can produce false hits. */
    unsigned top=meta&CROP_TOP_MASK,bottom=top+b->crop_height[slot];
    unsigned column=(meta&CROP_RIGHT)?4u:0u;
    for(unsigned y=0;y<top;++y)if(row_nonzero(src+y*8u))return 0;
    for(unsigned y=top;y<bottom;++y){
        const uint8_t *row=src+y*8u;
        if(half_nonzero(row+(column^4u)))return 0;
        row+=column;
        if((dst[0]^row[0])|(dst[1]^row[1])|(dst[2]^row[2])|(dst[3]^row[3]))return 0;
        dst+=4u;
    }
    for(unsigned y=bottom;y<16u;++y)if(row_nonzero(src+y*8u))return 0;
    return 1;
}
#endif

wf_status_t wf_saturn_yaul_rom_frame_prepare(wf_saturn_yaul_backend_t*b,const wf_vdp1_frame_plan_t*p){
 if(!b||!p||b->active||p->tile_count>WF_VDP1_STREAM_TILES_MAX)return WF_BAD_STATE;
#ifdef WF_PLATFORM_SATURN
 if(g_rom_owner!=b){wp1980_vram_cache_init(&g_rom_cache);g_rom_owner=b;}
 uint16_t ids[WF_VDP1_STREAM_TILES_MAX];
 for(unsigned i=0;i<p->tile_count;i++)ids[i]=p->tile_upload[i].source_tile;
 if(wp1980_vram_cache_prepare(&g_rom_cache,ids,p->tile_count))return WF_BAD_STATE;
 wp1980_vram_hits=g_rom_cache.hits;wp1980_vram_misses=g_rom_cache.misses;wp1980_vram_evictions=g_rom_cache.evictions;
 g_rom_prepared=b;
#endif
 return WF_OK;
}

static wf_status_t begin_frame(void *ctx,unsigned nt,unsigned np,unsigned nc){
    wf_saturn_yaul_backend_t*b=(wf_saturn_yaul_backend_t*)ctx;
    if(!b||b->active)return WF_BAD_STATE;
    if(nt>WF_VDP1_STREAM_TILES_MAX||np>WF_SATURN_CRAM_SPRITE_BANKS||nc>WF_VDP1_COMMANDS_MAX)return WF_BLOCKED;
    if(nt>WF_SATURN_VDP1_STREAM_TILE_CAPACITY||nc+4u+1u>WF_SATURN_VDP1_CMD_COUNT)return WF_BLOCKED;
#ifdef WF_PLATFORM_SATURN
    if(nt&&(!b->hwr_stage||b->hwr_stage_bytes<(size_t)nt*WF_VDP1_TILE_BYTES_16X16_4BPP))return WF_BLOCKED;
#endif
#ifdef WF_PLATFORM_SATURN
    unsigned rom_frame=g_rom_prepared==b;
    g_rom_prepared=NULL;
    if(rom_frame&&g_rom_cache.frame_count!=nt)return WF_BAD_STATE;
#if WP1980_VRAM_POOL_END>WF_SATURN_VDP1_DYNAMIC_OFFSET
    if(!rom_frame&&b->rom_frame)wp1980_vram_cache_invalidate(&g_rom_cache);
#endif
    if(b->rom_frame!=rom_frame){memset(b->resident,0,sizeof b->resident);memset(b->rom_identity,0,sizeof b->rom_identity);}
    b->rom_frame=(uint8_t)rom_frame;
#endif
    b->expected_tiles=nt;b->expected_palettes=np;b->expected_cmds=nc;
    b->emitted_tiles=b->emitted_palettes=b->emitted_cmds=0;b->active=1;
    /* Clearing dirty also invalidates crop bounds for uncommitted staging
     * left by an aborted/incomplete frame: those slots are not resident. */
    memset(b->dirty,0,sizeof b->dirty);b->palette_mask=0;
#ifdef WF_PLATFORM_SATURN
    /* Initialize the two control commands only. Each sprite and END is
     * fully initialized when emitted; the unused tail is never submitted. */
    memset(g_cmdts,0,2u*sizeof(g_cmdts[0]));g_cmd_index=0;
    vdp1_cmdt_system_clip_coord_set(&g_cmdts[g_cmd_index]);vdp1_cmdt_vtx_system_clip_coord_set(&g_cmdts[g_cmd_index++],INT16_VEC2(319,239));
    vdp1_cmdt_local_coord_set(&g_cmdts[g_cmd_index]);vdp1_cmdt_vtx_local_coord_set(&g_cmdts[g_cmd_index++],INT16_VEC2(0,-8));
    /* Map the original visible source Y8..247 onto physical Y0..239. */
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
    unsigned mask=byte_mask(slot);uint8_t *dst=b->hwr_stage+(size_t)slot*bytes;
    if((b->resident[slot>>3]&mask)&&tile_matches(b,slot,src,dst))++b->texture_reused;
    else {
        memcpy(dst,src,bytes);b->dirty[slot>>3]|=(uint8_t)mask;
        /* Analyze/compact HWRAM only on a changed upload. ROM identity hits
         * above this path never decode or scan a tile. */
        cache_crop(b,slot,dst);
        /* Staging no longer describes VRAM until end_frame commits it. */
        b->resident[slot>>3]&=(uint8_t)~mask;
    }
#else
    (void)off;(void)bytes;
#endif
    b->rom_identity[slot>>3]&=(uint8_t)~(byte_mask(slot));
    b->emitted_tiles++;return WF_OK;
}

wf_status_t wf_saturn_yaul_upload_rom_tile(wf_saturn_yaul_backend_t*b,uint16_t slot,uint16_t source,wf_status_t (*read_tile)(uint16_t,const uint8_t**)){
 if(!b||!b->active||!read_tile||slot!=b->emitted_tiles||slot>=b->expected_tiles)return WF_BAD_STATE;
 unsigned mask=byte_mask(slot);
#ifdef WF_PLATFORM_SATURN
 if(b->rom_frame){
  unsigned physical=g_rom_cache.frame_slot[slot];
  if(physical>=WP1980_VRAM_CACHE_SLOTS||g_rom_cache.source[physical]!=source)return WF_BAD_STATE;
  if(g_rom_cache.flags[physical]&WP1980_VRAM_COMMITTED){
   b->crop_meta[slot]=g_rom_cache.crop_meta[physical];b->crop_height[slot]=g_rom_cache.crop_height[physical];
   b->rom_source[slot]=source;b->rom_identity[slot>>3]|=(uint8_t)mask;b->resident[slot>>3]|=(uint8_t)mask;
   ++b->emitted_tiles;++b->texture_reused;return WF_OK;
  }
  /* A new physical slot must receive bytes even if old dense staging happens
   * to contain an identical picture. Commit identity only after its DMA. */
  b->resident[slot>>3]&=(uint8_t)~mask;
 }
#endif
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
    if(!b||!b->active||!c||c->stream_tile>=b->expected_tiles)return WF_BAD_STATE;
    if(c->cram_slot>=WF_SATURN_CRAM_SPRITE_BANKS)return WF_BLOCKED;
#ifdef WF_PLATFORM_SATURN
    if(b->rom_frame)off=WP1980_VRAM_POOL_OFFSET+(uint32_t)g_rom_cache.frame_slot[c->stream_tile]*128u;
    else
#endif
    if(wf_saturn_stream_tile_offset(c->stream_tile,&off)!=WF_OK)return WF_BLOCKED;
#ifdef WF_PLATFORM_SATURN
    unsigned top=0,height=16,left=0,width=16;
    /* VRAM accepts512 slots, while staging and these bounds own256. Slots
     * without valid staged/resident bytes retain the complete16-row sprite. */
    if(c->stream_tile<WF_VDP1_STREAM_TILES_MAX){
        unsigned slot=c->stream_tile,mask=byte_mask(slot);
        if((b->resident[slot>>3]|b->dirty[slot>>3])&mask){
            unsigned meta=b->crop_meta[slot];
            top=meta&CROP_TOP_MASK;height=b->crop_height[slot];
            if(meta&CROP_NARROW){width=8u;left=(meta&CROP_RIGHT)?8u:0u;}
        }
    }
    /* Pen0 remains transparent. An empty source has no hardware work, but
     * it still fulfils one requested command in the submission contract. */
    if(!height){++b->emitted_cmds;return WF_OK;}
    if(g_cmd_index>=WF_VDP1_COMMANDS_MAX+3u)return WF_BLOCKED;
    vdp1_cmdt_t *cmd=&g_cmdts[g_cmd_index++];
    /* Write every hardware word once. A sprite is normal8/16x1..16,
     * CB16/replace with transparent pen0 and end-code disabled (penF is
     * a real arcade color). No full clear followed by rewriting fields. */
    cmd->cmd_ctrl=(uint16_t)((c->flip_x?0x10u:0u)|(c->flip_y?0x20u:0u));
    cmd->cmd_link=0;
    cmd->cmd_pmod=0x0080u;
    cmd->cmd_colr=(uint16_t)(WF_SATURN_CRAM_SPRITE_FIRST+(unsigned)c->cram_slot*16u);
    cmd->cmd_srca=(uint16_t)((off>>3)+(width==16u?top:0u));
    cmd->cmd_size=(uint16_t)((width==8u?0x0100u:0x0200u)|height);
    cmd->cmd_xa=(int16_t)(c->x+(int)(c->flip_x?16u-left-width:left));
    cmd->cmd_ya=(int16_t)(c->y+(int)(c->flip_y?16u-top-height:top));
    cmd->cmd_xb=cmd->cmd_yb=cmd->cmd_xc=cmd->cmd_yc=0;
    cmd->cmd_xd=cmd->cmd_yd=0;
    cmd->cmd_grda=cmd->reserved=0;
#else
    (void)off;
#endif
    b->emitted_cmds++;return WF_OK;
}

wf_status_t wf_saturn_yaul_emit_sprites(wf_saturn_yaul_backend_t*b,const wf_vdp1_sprite_cmd_t*commands,unsigned count){
    if(!b||!b->active||(!commands&&count)||b->emitted_tiles!=b->expected_tiles||
       b->expected_tiles>WF_VDP1_STREAM_TILES_MAX||b->emitted_cmds>b->expected_cmds||
       count>b->expected_cmds-b->emitted_cmds)return WF_BAD_STATE;
    unsigned i=0;
    wf_status_t status=WF_OK;
#ifdef WF_PLATFORM_SATURN
    unsigned index=g_cmd_index;
    if(index>WF_VDP1_COMMANDS_MAX+3u||count>WF_VDP1_COMMANDS_MAX+3u-index)return WF_BLOCKED;
    unsigned source_base=(b->rom_frame?WP1980_VRAM_POOL_OFFSET:WF_SATURN_VDP1_DYNAMIC_OFFSET)>>3;
    /* The table is32-byte aligned and each command occupies32 bytes. These
     * four aligned stores clear the unused tail without eight halfword
     * stores or a freestanding memset call. may_alias preserves C aliasing. */
    typedef uint32_t command_word_t __attribute__((__may_alias__));
    _Static_assert(sizeof(vdp1_cmdt_t)==32u&&offsetof(vdp1_cmdt_t,cmd_xb)==16u,"VDP1 command tail layout");
#endif
    for(;i<count;++i){
        const wf_vdp1_sprite_cmd_t*c=&commands[i];
        unsigned slot=c->stream_tile;
        if(slot>=b->expected_tiles){status=WF_BAD_STATE;break;}
        if(c->cram_slot>=WF_SATURN_CRAM_SPRITE_BANKS){status=WF_BLOCKED;break;}
#ifdef WF_PLATFORM_SATURN
        /* Every successful tile upload makes its crop bounds resident or
         * dirty. The entry check therefore replaces the per-sprite bitset
         * lookup; even an empty tile fulfils its logical command. */
        unsigned height=b->crop_height[slot];
        if(!height)continue;
        unsigned meta=b->crop_meta[slot],top=meta&CROP_TOP_MASK;
        unsigned width=(meta&CROP_NARROW)?8u:16u;
        unsigned left=(meta&CROP_NARROW)&&(meta&CROP_RIGHT)?8u:0u;
        unsigned physical=b->rom_frame?g_rom_cache.frame_slot[slot]:slot;
        vdp1_cmdt_t*cmd=&g_cmdts[index++];
        cmd->cmd_ctrl=(uint16_t)((c->flip_x?0x10u:0u)|(c->flip_y?0x20u:0u));
        cmd->cmd_link=0;
        cmd->cmd_pmod=0x0080u;
        cmd->cmd_colr=(uint16_t)(WF_SATURN_CRAM_SPRITE_FIRST+(unsigned)c->cram_slot*16u);
        cmd->cmd_srca=(uint16_t)(source_base+physical*16u+(width==16u?top:0u));
        cmd->cmd_size=(uint16_t)((width==8u?0x0100u:0x0200u)|height);
        cmd->cmd_xa=(int16_t)(c->x+(int)(c->flip_x?16u-left-width:left));
        cmd->cmd_ya=(int16_t)(c->y+(int)(c->flip_y?16u-top-height:top));
        command_word_t*words=(command_word_t*)(void*)cmd;
        words[4]=words[5]=words[6]=words[7]=0;
#endif
    }
#ifdef WF_PLATFORM_SATURN
    g_cmd_index=index;
#endif
    b->emitted_cmds+=i;
    return status;
}

static wf_status_t end_frame(void *ctx){
    wf_saturn_yaul_backend_t*b=(wf_saturn_yaul_backend_t*)ctx;
    if(!b||!b->active)return WF_BAD_STATE;
    if(b->emitted_tiles!=b->expected_tiles||b->emitted_palettes!=b->expected_palettes||b->emitted_cmds!=b->expected_cmds){b->active=0;return WF_BAD_STATE;}
#ifdef WF_PLATFORM_SATURN
    if(g_cmd_index>=WF_VDP1_COMMANDS_MAX+4u){b->active=0;return WF_BLOCKED;}
    /* Each DMA contains only a consecutive run whose resident bytes changed. */
    for(unsigned slot=0;slot<b->expected_tiles;){
        if(!(b->dirty[slot>>3]&(byte_mask(slot)))){++slot;continue;}
        unsigned first=slot++;
        unsigned physical=b->rom_frame?g_rom_cache.frame_slot[first]:first;
        while(slot<b->expected_tiles&&(b->dirty[slot>>3]&(byte_mask(slot)))&&
              (!b->rom_frame||g_rom_cache.frame_slot[slot]==physical+slot-first))++slot;
        size_t bytes=(size_t)(slot-first)*128u;
        unsigned destination=(b->rom_frame?WP1980_VRAM_POOL_OFFSET:WF_SATURN_VDP1_DYNAMIC_OFFSET)+physical*128u;
        scu_dma_transfer(0,(void*)VDP1_VRAM(destination),b->hwr_stage+first*128u,bytes);
        scu_dma_transfer_wait(0);b->texture_bytes+=(uint32_t)bytes;++b->texture_transfers;
        for(unsigned i=first;i<slot;i++){
            b->resident[i>>3]|=(uint8_t)(byte_mask(i));
            if(b->rom_frame&&wp1980_vram_cache_commit(&g_rom_cache,g_rom_cache.frame_slot[i],b->crop_meta[i],b->crop_height[i])){
                b->active=0;return WF_BAD_STATE;
            }
        }
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
    g_cmd_index=0; /* No command list is submitted by an aborted frame. */
#endif
}

static const wf_saturn_render_ops_t g_ops={begin_frame,upload_tile,upload_palette,emit_sprite,end_frame,abort_frame};
void wf_saturn_yaul_backend_init(wf_saturn_yaul_backend_t*b,void*h,size_t n){
 if(!b)return;
 memset(b,0,sizeof(*b));b->hwr_stage=(uint8_t*)h;b->hwr_stage_bytes=n;
#ifdef WF_PLATFORM_SATURN
 wp1980_vram_cache_init(&g_rom_cache);g_rom_owner=b;g_rom_prepared=NULL;
 wp1980_vram_hits=wp1980_vram_misses=wp1980_vram_evictions=0;
#endif
}
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
