#include <string.h>
#include "wf_saturn_yaul_backend.h"

/* SH-2 has no variable barrel shift. These byte-sized masks avoid a
 * software shift call at each tile/residency/dirty test in the hot path. */
static unsigned byte_mask(unsigned bit){
    static const uint8_t masks[8]={1,2,4,8,16,32,64,128};
    return masks[bit&7u];
}

volatile uint32_t wp2015_vdp1_bank_observer[7];

#ifdef WF_PLATFORM_SATURN
#include <yaul.h>
#include "wp2015_vdp1_observer.h"
#include "wp2015_display_pipeline.h"
static vdp1_cmdt_t g_cmdts[WF_VDP1_COMMANDS_MAX+4u] __aligned(32);
static unsigned g_cmd_index;
static volatile unsigned g_draw_bank,g_write_bank,g_ready,g_pending;
static volatile uint32_t g_submit_seq,g_publish_seq,g_ack_seq;

_Static_assert(1u+2u*WF_SATURN_VDP1_CMD_BANK_COUNT==1033u,"Root and two command banks end at 0x8120");
_Static_assert(WF_SATURN_VDP1_CMD_BANK_COUNT>=WF_VDP1_COMMANDS_MAX+4u,"Each bank holds the full command contract");
_Static_assert(2u*WF_SATURN_VDP1_TEXTURE_BANK_BYTES==WF_SATURN_VDP1_DYNAMIC_BYTES,"Both texture banks stay inside the existing lease");

static void publish_vblank_in(void *work){
    (void)work;
    wp2015_display_vblank_in(g_draw_bank,g_ack_seq);
    if(!g_pending)return;
    /* The hardware swaps and fetches the root before the VBlankOut CPU
     * callback. Publish in VBlankIn, never by rewriting it at VBlankOut.
     * A delayed IRQ must not rewrite the root after that fetch has begun.
     * Exclude the final four encoded lines, including Out at 0x1ff. */
    vdp2_tvmd_extern_latch();
    unsigned line=vdp2_tvmd_vcount_get();
    if(!vdp2_tvmd_vblank_in()||line<WF_SATURN_VIEW_H||line>=0x1fcu){
        ++wp2015_vdp1_bank_observer[5];return;
    }
    if(!(((volatile vdp1_ioregs_t*)VDP1_IOREG_BASE)->edsr&2u)){
        ++wp2015_vdp1_bank_observer[6];return;
    }
    ((volatile vdp1_cmdt_t*)VDP1_VRAM(0))->cmd_link=
        (uint16_t)((1u+g_write_bank*WF_SATURN_VDP1_CMD_BANK_COUNT)*4u);
    g_publish_seq=g_submit_seq;g_pending=0;
    ++wp2015_vdp1_bank_observer[0];
}

void wf_saturn_yaul_vblank_out(void){
    wp2015_display_vblank_out();
    if(g_publish_seq==g_ack_seq)return;
    /* This callback follows the hardware swap/root fetch. The previously
     * drawn bank can now be reused even if this new bank is still busy. */
    g_draw_bank=g_write_bank;g_ack_seq=g_publish_seq;
    ++wp2015_vdp1_bank_observer[1];
}

wf_status_t wf_saturn_yaul_frame_sync(void){
    unsigned mask=cpu_intc_mask_get();cpu_intc_mask_set(15);
    if(!g_ready||g_pending||g_submit_seq!=g_ack_seq){
        cpu_intc_mask_set(mask);return WF_BAD_STATE;
    }
    if(wp2015_display_seal(g_write_bank,g_submit_seq+1u)){cpu_intc_mask_set(mask);return WF_BAD_STATE;}
    g_ready=0;g_pending=1;++g_submit_seq;
    /* Keep both SDK requests in one interrupt exclusion window, avoiding
     * the former VBlankIn gap between Sync1 and Sync2. No DMA/wait here. */
    vdp1_sync();if(!wp2015_display_active())vdp2_sync();cpu_intc_mask_set(mask);
    return WF_OK;
}

void wf_saturn_yaul_frame_wait(void){
    vdp1_sync_wait();if(!wp2015_display_active())vdp2_sync_wait();
    /* SDK AUTO's flag can clear at an Out preceding publication when the
     * request arrived after In. Ack only the actual published generation. */
    while(g_ack_seq!=g_submit_seq){}
    (void)wp2015_display_prepare(g_draw_bank^1u);
}

void wf_saturn_yaul_display_drain(void){
    while(wp2015_display_active()&&wp2015_display_visible_generation()!=g_ack_seq){}
}

static unsigned row_nonzero(const uint8_t *row){
    if(!((uintptr_t)row&3u)){
        uint32_t a,b;
        __builtin_memcpy(&a,__builtin_assume_aligned(row,4u),4u);
        __builtin_memcpy(&b,__builtin_assume_aligned(row+4u,4u),4u);
        return a|b;
    }
    /* Generic callers may still provide unaligned staging/source bytes. */
    return (unsigned)row[0]|row[1]|row[2]|row[3]|row[4]|row[5]|row[6]|row[7];
}

enum { CROP_TOP_MASK=0x1fu,CROP_NARROW=0x20u,CROP_RIGHT=0x40u };
_Static_assert(WF_VDP1_TILE_BYTES_16X16_4BPP==128u,"Cropping owns one 16x16 4bpp tile");
_Static_assert(WF_SATURN_VDP1_STREAM_TILE_BYTES==128u,"VRAM slots retain their 128-byte stride");

static unsigned half_nonzero(const uint8_t *row){
    if(!((uintptr_t)row&3u)){
        uint32_t value;
        __builtin_memcpy(&value,__builtin_assume_aligned(row,4u),4u);
        return value;
    }
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

/* Same byte compaction as cache_crop, using bounds validated at load time.
 * The full 128-byte slot and all untouched trailing bytes remain identical. */
static void cache_crop_known(wf_saturn_yaul_backend_t*b,unsigned slot,uint8_t*tile,uint16_t packed){
    unsigned meta=packed>>8,height=packed&255u;
    if(meta&CROP_NARROW){
        unsigned top=meta&CROP_TOP_MASK,column=(meta&CROP_RIGHT)?4u:0u;
        uint8_t *dst=tile;const uint8_t *src=tile+top*8u+column;
        for(unsigned y=0;y<height;++y){
            dst[0]=src[0];dst[1]=src[1];dst[2]=src[2];dst[3]=src[3];
            dst+=4u;src+=8u;
        }
    }
    b->crop_meta[slot]=(uint8_t)meta;b->crop_height[slot]=(uint8_t)height;
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

static wf_status_t begin_frame(void *ctx,unsigned nt,unsigned np,unsigned nc){
    wf_saturn_yaul_backend_t*b=(wf_saturn_yaul_backend_t*)ctx;
    if(!b||b->active)return WF_BAD_STATE;
    if(nt>WF_VDP1_STREAM_TILES_MAX||np>WF_SATURN_CRAM_SPRITE_BANKS||nc>WF_VDP1_COMMANDS_MAX)return WF_BLOCKED;
    if(nt>WF_SATURN_VDP1_STREAM_TILE_CAPACITY||nc+4u+1u>WF_SATURN_VDP1_CMD_COUNT)return WF_BLOCKED;
#ifdef WF_PLATFORM_SATURN
    if(g_ready||g_pending||g_submit_seq!=g_ack_seq){++wp2015_vdp1_bank_observer[2];return WF_BAD_STATE;}
    if(nt&&(!b->hwr_stage||b->hwr_stage_bytes<(size_t)nt*WF_VDP1_TILE_BYTES_16X16_4BPP))return WF_BLOCKED;
    g_write_bank=g_draw_bank^1u;
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
    vdp1_cmdt_local_coord_set(&g_cmdts[g_cmd_index]);vdp1_cmdt_vtx_local_coord_set(&g_cmdts[g_cmd_index++],INT16_VEC2(0,0));
    /* VDP1's transparent erase exposes the lower-priority VDP2 ring.
     * An opaque clearing polygon would cover both bitmap layers. */
#endif
    return WF_OK;
}

static wf_status_t upload_tile_crop(void *ctx,uint16_t slot,const uint8_t*src,size_t bytes,uint16_t crop){
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
        if(!(((uintptr_t)dst|(uintptr_t)src)&3u))
            __builtin_memcpy(__builtin_assume_aligned(dst,4u),
                             __builtin_assume_aligned(src,4u),128u);
        else memcpy(dst,src,bytes);
        b->dirty[slot>>3]|=(uint8_t)mask;
        b->bank_stale[0][slot>>3]|=(uint8_t)mask;
        b->bank_stale[1][slot>>3]|=(uint8_t)mask;
        /* Analyze/compact HWRAM only on a changed upload. ROM identity hits
         * above this path never decode or scan a tile. */
        if(crop==0xffffu)cache_crop(b,slot,dst);else cache_crop_known(b,slot,dst,crop);
        /* Staging no longer describes VRAM until end_frame commits it. */
        b->resident[slot>>3]&=(uint8_t)~mask;
    }
#else
    (void)off;(void)bytes;(void)crop;
#endif
    b->rom_identity[slot>>3]&=(uint8_t)~(byte_mask(slot));
    b->emitted_tiles++;return WF_OK;
}

static wf_status_t upload_tile(void *ctx,uint16_t slot,const uint8_t*src,size_t bytes){
    return upload_tile_crop(ctx,slot,src,bytes,0xffffu);
}

wf_status_t wf_saturn_yaul_upload_rom_tile_crop(wf_saturn_yaul_backend_t*b,uint16_t slot,uint16_t source,wf_status_t (*read_tile)(uint16_t,const uint8_t**),uint16_t (*read_crop)(uint16_t)){
 if(!b||!b->active||!read_tile||slot!=b->emitted_tiles||slot>=b->expected_tiles)return WF_BAD_STATE;
 unsigned mask=byte_mask(slot);
 if((b->resident[slot>>3]&mask)&&(b->rom_identity[slot>>3]&mask)&&b->rom_source[slot]==source){
  ++b->emitted_tiles;++b->texture_reused;return WF_OK;
 }
 const uint8_t *p;wf_status_t s=read_tile(source,&p);if(s!=WF_OK)return s;
 s=upload_tile_crop(b,slot,p,128,read_crop?read_crop(source):0xffffu);
 if(s==WF_OK){b->rom_source[slot]=source;b->rom_identity[slot>>3]|=(uint8_t)mask;}
 return s;
}


/* The presenter supplies dense slots starting at zero, exactly as its
 * former scalar loop did. This leaves all decoding, generic crop, stage
 * changes and error handling in the existing scalar miss path. */
wf_status_t wf_saturn_yaul_upload_rom_tiles(wf_saturn_yaul_backend_t*b,
    const wf_vdp1_tile_upload_t*tiles,size_t count,
    wf_status_t (*read_tile)(uint16_t,const uint8_t**),uint16_t (*read_crop)(uint16_t)){
    if(!count)return WF_OK;
    if(!b||!b->active||!tiles||!read_tile)return WF_BAD_STATE;
    for(size_t i=0;i<count;i++){
        uint16_t slot=(uint16_t)i,source=tiles[i].source_tile;
        if(slot!=b->emitted_tiles||slot>=b->expected_tiles)return WF_BAD_STATE;
        unsigned byte=slot>>3,mask=byte_mask(slot);
        if((b->resident[byte]&b->rom_identity[byte]&mask)&&b->rom_source[slot]==source){
            ++b->emitted_tiles;++b->texture_reused;
        }else{
            /* A failing read/crop/upload preserves the same completed
             * prefix and callback order as the scalar presentation loop. */
            wf_status_t status=wf_saturn_yaul_upload_rom_tile_crop(b,slot,source,read_tile,read_crop);
            if(status!=WF_OK)return status;
        }
    }
    return WF_OK;
}

wf_status_t wf_saturn_yaul_upload_rom_tile(wf_saturn_yaul_backend_t*b,uint16_t slot,uint16_t source,wf_status_t (*read_tile)(uint16_t,const uint8_t**)){
 return wf_saturn_yaul_upload_rom_tile_crop(b,slot,source,read_tile,NULL);
}

static wf_status_t upload_palette(void *ctx,uint8_t bank,const uint8_t*src,size_t bytes){
    wf_saturn_yaul_backend_t*b=(wf_saturn_yaul_backend_t*)ctx;
    if(!b||!b->active||!src||bytes!=WF_VDP1_PALETTE_BYTES||bank>=WF_SATURN_CRAM_SPRITE_BANKS)return WF_BAD_STATE;
    if(b->emitted_palettes>=b->expected_palettes||(b->palette_mask&(1u<<bank)))return WF_BAD_STATE;
    memcpy(b->palette_stage[bank],src,32);b->palette_mask|=(uint16_t)(1u<<bank);
    b->emitted_palettes++;return WF_OK;
}

#ifdef WF_PLATFORM_SATURN
/* Both entry points use the same command writer. The batch path folds the
 * immutable address stride and bank base once; crop and error-prefix
 * semantics remain those of an individual accepted command. */
static inline __attribute__((always_inline)) wf_status_t emit_sprite_native(
    wf_saturn_yaul_backend_t*b,const wf_vdp1_sprite_cmd_t*c,unsigned tile_srca){
    unsigned top=0,height=16,left=0,width=16;
    /* Bounds describe this logical slot only when its staged or resident
     * bytes are valid; otherwise retain the complete 16-row sprite. */
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
    cmd->cmd_srca=(uint16_t)(tile_srca+(width==16u?top:0u));
    cmd->cmd_size=(uint16_t)((width==8u?0x0100u:0x0200u)|height);
    cmd->cmd_xa=(int16_t)(c->x+(int)(c->flip_x?16u-left-width:left));
    cmd->cmd_ya=(int16_t)(c->y+(int)(c->flip_y?16u-top-height:top));
    cmd->cmd_xb=cmd->cmd_yb=cmd->cmd_xc=cmd->cmd_yc=0;
    cmd->cmd_xd=cmd->cmd_yd=0;
    cmd->cmd_grda=cmd->reserved=0;
    ++b->emitted_cmds;return WF_OK;
}
#endif

static wf_status_t emit_sprite(void *ctx,const wf_vdp1_sprite_cmd_t*c){
    wf_saturn_yaul_backend_t*b=(wf_saturn_yaul_backend_t*)ctx;uint32_t off;
    if(!b||!b->active||!c||c->stream_tile>=b->expected_tiles)return WF_BAD_STATE;
    if(c->cram_slot>=WF_SATURN_CRAM_SPRITE_BANKS)return WF_BLOCKED;
    if(wf_saturn_stream_tile_offset(c->stream_tile,&off)!=WF_OK)return WF_BLOCKED;
#ifdef WF_PLATFORM_SATURN
    off+=g_write_bank*WF_SATURN_VDP1_TEXTURE_BANK_BYTES;
    return emit_sprite_native(b,c,off>>3);
#else
    (void)off;++b->emitted_cmds;return WF_OK;
#endif
}

wf_status_t wf_saturn_yaul_emit_sprites(wf_saturn_yaul_backend_t*b,
    const wf_vdp1_sprite_cmd_t*commands,size_t count){
    /* An empty sequence has no scalar calls, including no context check. */
    if(!count)return WF_OK;
    if(!b||!b->active||!commands)return WF_BAD_STATE;
#ifdef WF_PLATFORM_SATURN
    _Static_assert(WF_SATURN_VDP1_STREAM_TILE_BYTES%8u==0u,"VDP1 source units are eight bytes");
    _Static_assert(WF_SATURN_VDP1_DYNAMIC_OFFSET+
        WF_SATURN_VDP1_STREAM_TILE_CAPACITY*WF_SATURN_VDP1_STREAM_TILE_BYTES<=
        WF_SATURN_VDP1_DYNAMIC_END,"Each accepted slot is inside the texture lease");
    /* begin_frame owns this inactive bank until end_frame seals it. The
     * publication IRQ cannot replace g_write_bank during this loop. */
    unsigned base=(WF_SATURN_VDP1_DYNAMIC_OFFSET+
        g_write_bank*WF_SATURN_VDP1_TEXTURE_BANK_BYTES)>>3;
    for(size_t i=0;i<count;i++){
        const wf_vdp1_sprite_cmd_t*c=commands+i;
        /* Validate in scalar order and stop after the same accepted prefix.
         * Empty tiles are accepted even when the hardware command list is full. */
        if(c->stream_tile>=b->expected_tiles)return WF_BAD_STATE;
        if(c->cram_slot>=WF_SATURN_CRAM_SPRITE_BANKS)return WF_BLOCKED;
        if(c->stream_tile>=WF_SATURN_VDP1_STREAM_TILE_CAPACITY)return WF_BLOCKED;
        wf_status_t status=emit_sprite_native(b,c,
            base+(unsigned)c->stream_tile*(WF_SATURN_VDP1_STREAM_TILE_BYTES/8u));
        if(status!=WF_OK)return status;
    }
#else
    for(size_t i=0;i<count;i++){
        wf_status_t status=emit_sprite(b,commands+i);
        if(status!=WF_OK)return status;
    }
#endif
    return WF_OK;
}

static wf_status_t end_frame(void *ctx){
    wf_saturn_yaul_backend_t*b=(wf_saturn_yaul_backend_t*)ctx;
    if(!b||!b->active)return WF_BAD_STATE;
    if(b->emitted_tiles!=b->expected_tiles||b->emitted_palettes!=b->expected_palettes||b->emitted_cmds!=b->expected_cmds){b->active=0;return WF_BAD_STATE;}
#ifdef WF_PLATFORM_SATURN
    if(g_cmd_index>=WF_VDP1_COMMANDS_MAX+4u){b->active=0;return WF_BLOCKED;}
    wp2015_vdp1_observe_begin();
    /* Stale bits survive frame abort/dirty reset: the other physical bank
     * can still contain an older logical slot even after an identity hit. */
    uint8_t *stale=b->bank_stale[g_write_bank];
    for(unsigned slot=0;slot<b->expected_tiles;){
        if(!(stale[slot>>3]&(byte_mask(slot)))){++slot;continue;}
        unsigned first=slot++;
        while(slot<b->expected_tiles&&(stale[slot>>3]&(byte_mask(slot))))++slot;
        size_t bytes=(size_t)(slot-first)*128u;
        if(g_write_bank==g_draw_bank){++wp2015_vdp1_bank_observer[3];b->active=0;wp2015_vdp1_observe_end();return WF_BAD_STATE;}
        wp2015_vdp1_observe_dma(0);
        scu_dma_transfer(0,(void*)VDP1_VRAM(WF_SATURN_VDP1_DYNAMIC_OFFSET+g_write_bank*WF_SATURN_VDP1_TEXTURE_BANK_BYTES+first*128u),b->hwr_stage+first*128u,bytes);
        scu_dma_transfer_wait(0);b->texture_bytes+=(uint32_t)bytes;++b->texture_transfers;
        for(unsigned i=first;i<slot;i++){
            unsigned mask=byte_mask(i);b->resident[i>>3]|=(uint8_t)mask;
            stale[i>>3]&=(uint8_t)~mask;
        }
    }
    for(unsigned bank=0;bank<16;bank++)if(b->palette_mask&(1u<<bank))
        wf_saturn_cram_write_bank((uint16_t)(WF_SATURN_CRAM_SPRITE_FIRST+bank*16u),b->palette_stage[bank]);
    memset(&g_cmdts[g_cmd_index],0,sizeof(g_cmdts[g_cmd_index]));vdp1_cmdt_end_set(&g_cmdts[g_cmd_index]);g_cmd_index++;
    vdp1_cmdt_list_t list;vdp1_cmdt_list_init(&list,g_cmdts);list.count=(uint16_t)g_cmd_index;
    if(g_write_bank==g_draw_bank){++wp2015_vdp1_bank_observer[4];b->active=0;wp2015_vdp1_observe_end();return WF_BAD_STATE;}
    wp2015_vdp1_observe_dma(1);
    vdp1_sync_cmdt_list_put(&list,(uint16_t)(1u+g_write_bank*WF_SATURN_VDP1_CMD_BANK_COUNT));vdp1_sync_put_wait();
    wp2015_vdp1_observe_end();
    vdp1_sync_render();
    g_ready=1;
    if(!b->defer_sync){if(wf_saturn_yaul_frame_sync()!=WF_OK){b->active=0;return WF_BAD_STATE;}wf_saturn_yaul_frame_wait();}
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
    if(g_submit_seq==g_ack_seq)g_ready=0;
#endif
}

static const wf_saturn_render_ops_t g_ops={begin_frame,upload_tile,upload_palette,emit_sprite,end_frame,abort_frame};
void wf_saturn_yaul_backend_init(wf_saturn_yaul_backend_t*b,void*h,size_t n){if(!b)return;memset(b,0,sizeof(*b));b->hwr_stage=(uint8_t*)h;b->hwr_stage_bytes=n;memset(b->bank_stale,255,sizeof b->bank_stale);}
const wf_saturn_render_ops_t*wf_saturn_yaul_render_ops(void){return &g_ops;}
wf_status_t wf_saturn_yaul_present_plan(wf_saturn_yaul_backend_t*b,const wf_vdp1_frame_plan_t*p,const wf_vdp1_assets_t*a){return wf_saturn_render_submit(p,a,&g_ops,b);}

wf_status_t wf_saturn_yaul_video_init(void){
    if(wf_saturn_memory_contract_validate()!=WF_OK)return WF_BAD_STATE;
#ifdef WF_PLATFORM_SATURN
    wp2015_display_reset();
    wf_saturn_cram_reset();
    vdp2_cram_mode_set(WF_SATURN_CRAM_MODE);
    vdp1_vram_partitions_set(WF_SATURN_VDP1_CMD_COUNT,0x53460u,0,0);
    vdp1_env_t env;vdp1_env_default_init(&env);env.bpp=VDP1_ENV_BPP_16;env.color_mode=VDP1_ENV_COLOR_MODE_PALETTE;env.sprite_type=0;env.erase_color=RGB1555(0,0,0,0);env.erase_points[0]=(int16_vec2_t){.x=0,.y=0};env.erase_points[1]=(int16_vec2_t){.x=319,.y=239};vdp1_env_set(&env);
    vdp1_sync_interval_set(0);vdp1_sync_mode_set(VDP1_SYNC_MODE_ERASE_CHANGE);
    /* Boot-only setup, before any combat list is submitted. The live root
     * has one atomic publication word; both initial lists are valid END. */
    volatile uint16_t *root=(volatile uint16_t*)VDP1_VRAM(0);
    for(unsigned i=0;i<16u;++i)root[i]=0;
    root[1]=4u;root[0]=0x5000u;
    *(volatile uint16_t*)VDP1_VRAM(32u)=0x8000u;
    *(volatile uint16_t*)VDP1_VRAM((1u+WF_SATURN_VDP1_CMD_BANK_COUNT)*32u)=0x8000u;
    g_draw_bank=0;g_write_bank=1;g_ready=g_pending=0;
    g_submit_seq=g_publish_seq=g_ack_seq=0;
    memset((void*)wp2015_vdp1_bank_observer,0,sizeof wp2015_vdp1_bank_observer);
    vdp_sync_vblank_in_set(publish_vblank_in,NULL);
    ((volatile vdp1_ioregs_t*)VDP1_IOREG_BASE)->ptmr=2u;
    vdp2_sprite_priority_set(0,3);
#endif
    return WF_OK;
}
