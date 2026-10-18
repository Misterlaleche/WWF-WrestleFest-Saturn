#include "wf_graphics.h"
#include "wf_renderer_contract.h"
static const uint8_t gset[12]={0,1,2,3,10,11,6,7,8,9,4,5};
static uint16_t be16(const uint8_t *p){return (uint16_t)(((uint16_t)p[0]<<8)|p[1]);}
static uint32_t be32(const uint8_t *p){return ((uint32_t)p[0]<<24)|((uint32_t)p[1]<<16)|((uint32_t)p[2]<<8)|p[3];}
static int16_t bes16(const uint8_t *p){return (int16_t)be16(p);}
uint8_t wf_graphics_set_for_internal_id(uint8_t id){return id<12u?gset[id]:0xFFu;}
wf_status_t wf_graphics_emit_composition(const uint8_t *piece_be,size_t piece_bytes,uint32_t start,uint16_t count,int16_t source_x,int16_t source_y,uint8_t mirror,uint8_t facing,wf_draw_list_t *out){
    if(!piece_be||!out||count==0u)return WF_BLOCKED;
    if((size_t)(start+count)*10u>piece_bytes)return WF_BLOCKED;
    if(out->count+count>WF_DRAW_PIECES_MAX)return WF_BLOCKED;
    for(uint16_t i=0;i<count;i++){
        const uint8_t *p=piece_be+(size_t)(start+i)*10u; wf_draw_piece_t *d=&out->p[out->count++];
        int16_t rx=bes16(p+4),ry=bes16(p+6); uint8_t flags=p[3];
        int fx=flags&1u,fy=(flags>>1)&1u; if(mirror){rx=(int16_t)-rx;fx^=1;} if(facing){rx=(int16_t)-rx;fx^=1;}
        d->tile=be16(p);d->palette=p[2];d->flip_x=(uint8_t)fx;d->flip_y=(uint8_t)fy;
        /* WP35/WP26: source_x/source_y are intermediate $247C coordinates.
         * WP36 local offsets are applied before the 9-bit buffered-sprite
         * decode.  The source-space Y delta has the opposite sign of rel_y. */
        d->x=(int16_t)wf_sprite_x_decode((uint16_t)((uint16_t)source_x+(uint16_t)rx));
        d->y=(int16_t)wf_sprite_y_decode((uint16_t)((uint16_t)source_y-(uint16_t)ry));
        d->chain_index=p[8];d->chain=p[9];
    }
    return WF_OK;
}

wf_status_t wf_graphics_emit_actor(const wf_actor_raw_t *a,const wf_actor_visual_t *v,const wf_graphics_blob_t *b,wf_draw_list_t *out){
    uint8_t set=wf_graphics_set_for_internal_id(wf_u8(a,WF_A_CHARACTER)); if(set==0xFFu)return WF_BLOCKED;
    if(v->pose>=WF_COMBAT_POSES_PER_SET)return WF_BLOCKED;
    size_t idx=((size_t)set*WF_COMBAT_POSES_PER_SET+v->pose)*6u; if(idx+6u>b->pose_index_bytes)return WF_BLOCKED;
    uint32_t start=be32(b->pose_index_be+idx); uint16_t count=be16(b->pose_index_be+idx+4u); if(count==0u)return WF_BLOCKED;
    return wf_graphics_emit_composition(b->piece_be,b->piece_bytes,start,count,v->screen_x,v->screen_y,v->anim.pose_flag15,(uint8_t)((wf_u8(a,WF_A_FACING)&0x80u)!=0u),out);
}
