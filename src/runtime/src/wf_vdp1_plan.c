#include <string.h>
#include "wf_vdp1_plan.h"

void wf_vdp1_plan_reset(wf_vdp1_frame_plan_t *p){memset(p,0,sizeof(*p));}

static int tile_slot(const wf_vdp1_frame_plan_t *p,uint16_t tile){
    for(unsigned i=0;i<p->tile_count;i++) if(p->tile_upload[i].source_tile==tile)return (int)i;
    return -1;
}
static int pal_slot(const wf_vdp1_frame_plan_t *p,uint8_t pal){
    for(unsigned i=0;i<p->palette_count;i++) if(p->palette_upload[i].source_resource==pal)return (int)i;
    return -1;
}
wf_status_t wf_vdp1_plan_build(const wf_draw_list_t *draw,const wf_vdp1_assets_t *a,wf_vdp1_frame_plan_t *p){
    if(!draw||!a||!p||!a->sprite_tiles||!a->palette_resources)return WF_BAD_STATE;
    wf_vdp1_plan_reset(p);
    if(draw->count>WF_VDP1_COMMANDS_MAX)return WF_BLOCKED;
    for(unsigned i=0;i<draw->count;i++){
        const wf_draw_piece_t *d=&draw->p[i];
        size_t tso=(size_t)d->tile*WF_VDP1_TILE_BYTES_16X16_4BPP;
        size_t pso=(size_t)d->palette*WF_VDP1_PALETTE_BYTES;
        if(tso+WF_VDP1_TILE_BYTES_16X16_4BPP>a->sprite_tile_bytes)return WF_BLOCKED;
        if(pso+WF_VDP1_PALETTE_BYTES>a->palette_bytes)return WF_BLOCKED;
        int ts=tile_slot(p,d->tile);
        if(ts<0){
            if(p->tile_count>=WF_VDP1_STREAM_TILES_MAX)return WF_BLOCKED;
            ts=(int)p->tile_count;
            wf_vdp1_tile_upload_t *u=&p->tile_upload[p->tile_count++];
            u->source_tile=d->tile;u->stream_slot=(uint16_t)ts;
            u->src_byte_offset=(uint32_t)tso;
            u->dst_byte_offset=(uint32_t)((size_t)ts*WF_VDP1_TILE_BYTES_16X16_4BPP);
            p->tile_upload_bytes+=WF_VDP1_TILE_BYTES_16X16_4BPP;
        }
        int ps=pal_slot(p,d->palette);
        if(ps<0){
            if(p->palette_count>=WF_VDP1_STREAM_PALETTES_MAX)return WF_BLOCKED;
            ps=(int)p->palette_count;
            wf_vdp1_palette_upload_t *u=&p->palette_upload[p->palette_count++];
            u->source_resource=d->palette;u->cram_slot=(uint8_t)ps;u->reserved=0;u->src_byte_offset=(uint32_t)pso;
            p->palette_upload_bytes+=WF_VDP1_PALETTE_BYTES;
        }
        wf_vdp1_sprite_cmd_t *c=&p->cmd[p->cmd_count++];
        c->x=d->x;c->y=d->y;c->stream_tile=(uint16_t)ts;c->cram_slot=(uint8_t)ps;
        c->flip_x=d->flip_x;c->flip_y=d->flip_y;c->chain_index=d->chain_index;c->chain=d->chain;
    }
    return wf_vdp1_plan_validate(p,a);
}
wf_status_t wf_vdp1_plan_validate(const wf_vdp1_frame_plan_t *p,const wf_vdp1_assets_t *a){
    if(!p||!a)return WF_BAD_STATE;
    if(p->tile_count>WF_VDP1_STREAM_TILES_MAX||p->palette_count>WF_VDP1_STREAM_PALETTES_MAX||p->cmd_count>WF_VDP1_COMMANDS_MAX)return WF_BAD_STATE;
    if(p->tile_upload_bytes!=(size_t)p->tile_count*WF_VDP1_TILE_BYTES_16X16_4BPP)return WF_BAD_STATE;
    if(p->palette_upload_bytes!=(size_t)p->palette_count*WF_VDP1_PALETTE_BYTES)return WF_BAD_STATE;
    for(unsigned i=0;i<p->tile_count;i++){
        const wf_vdp1_tile_upload_t *u=&p->tile_upload[i];
        if(u->stream_slot!=i)return WF_BAD_STATE;
        if((size_t)u->src_byte_offset+WF_VDP1_TILE_BYTES_16X16_4BPP>a->sprite_tile_bytes)return WF_BAD_STATE;
    }
    for(unsigned i=0;i<p->palette_count;i++){
        const wf_vdp1_palette_upload_t *u=&p->palette_upload[i];
        if(u->cram_slot!=i)return WF_BAD_STATE;
        if((size_t)u->src_byte_offset+WF_VDP1_PALETTE_BYTES>a->palette_bytes)return WF_BAD_STATE;
    }
    for(unsigned i=0;i<p->cmd_count;i++) if(p->cmd[i].stream_tile>=p->tile_count||p->cmd[i].cram_slot>=p->palette_count)return WF_BAD_STATE;
    return WF_OK;
}

wf_status_t wf_vdp1_plan_retain_tile_slots(wf_vdp1_frame_plan_t *p,
    const uint16_t *resident_tiles,const uint8_t *resident_bits,unsigned resident_count){
    /* Scratch is static because the Saturn master stack is deliberately small.
     * This renderer is serialized by the presentation owner. */
    /* The top seven bits are a generation, the low nine are slot+1.
     * Recycle once per 127 calls instead of clearing 1 KiB every frame. */
    static uint16_t lookup[512],slot_of[WF_VDP1_STREAM_TILES_MAX];
    static uint8_t lookup_epoch;
    static uint16_t source_at[WF_VDP1_STREAM_TILES_MAX];
    static uint8_t occupied[WF_VDP1_STREAM_TILES_MAX/8u];
    if(!p||!resident_tiles||!resident_bits||p->tile_count>WF_VDP1_STREAM_TILES_MAX||
       resident_count>WF_VDP1_STREAM_TILES_MAX||p->cmd_count>WF_VDP1_COMMANDS_MAX)return WF_BAD_STATE;
    unsigned count=p->tile_count;
    if(++lookup_epoch==128u){memset(lookup,0,sizeof lookup);lookup_epoch=1u;}
    memset(occupied,0,sizeof occupied);
    for(unsigned i=0;i<count;i++){
        uint16_t source=p->tile_upload[i].source_tile;
        unsigned hash=((uint32_t)source*0x9e3779b1u)>>23;
        while((lookup[hash]>>9)==lookup_epoch){
            unsigned old=((unsigned)lookup[hash]&511u)-1u;
            if(p->tile_upload[old].source_tile==source)return WF_BAD_STATE;
            hash=(hash+1u)&511u;
        }
        lookup[hash]=(uint16_t)(((unsigned)lookup_epoch<<9)|(i+1u));slot_of[i]=0xffffu;
    }
    for(unsigned i=0;i<p->cmd_count;i++)if(p->cmd[i].stream_tile>=count)return WF_BAD_STATE;
    unsigned limit=resident_count<count?resident_count:count;
    for(unsigned slot=0;slot<limit;slot++){
        if(!(resident_bits[slot>>3]&(uint8_t)(1u<<(slot&7u))))continue;
        uint16_t source=resident_tiles[slot];
        unsigned hash=((uint32_t)source*0x9e3779b1u)>>23;
        while((lookup[hash]>>9)==lookup_epoch){
            unsigned old=((unsigned)lookup[hash]&511u)-1u;
            if(p->tile_upload[old].source_tile==source){
                /* A source can remain resident in several old slots after
                 * previous compactions. Claim only one, or the free-slot
                 * count would no longer match the unmatched current tiles. */
                if(slot_of[old]==0xffffu){
                    slot_of[old]=(uint16_t)slot;occupied[slot>>3]|=(uint8_t)(1u<<(slot&7u));
                }
                break;
            }
            hash=(hash+1u)&511u;
        }
    }
    unsigned vacant=0;
    for(unsigned i=0;i<count;i++){
        if(slot_of[i]!=0xffffu)continue;
        while(occupied[vacant>>3]&(uint8_t)(1u<<(vacant&7u)))++vacant;
        slot_of[i]=(uint16_t)vacant;occupied[vacant>>3]|=(uint8_t)(1u<<(vacant&7u));
    }
    for(unsigned i=0;i<count;i++)source_at[slot_of[i]]=p->tile_upload[i].source_tile;
    for(unsigned i=0;i<count;i++){
        uint16_t source=source_at[i];
        p->tile_upload[i].source_tile=source;p->tile_upload[i].stream_slot=(uint16_t)i;
        p->tile_upload[i].src_byte_offset=(uint32_t)source*WF_VDP1_TILE_BYTES_16X16_4BPP;
        p->tile_upload[i].dst_byte_offset=i*WF_VDP1_TILE_BYTES_16X16_4BPP;
    }
    for(unsigned i=0;i<p->cmd_count;i++)p->cmd[i].stream_tile=slot_of[p->cmd[i].stream_tile];
    return WF_OK;
}
