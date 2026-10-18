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
