#include "wf_saturn_render_bridge.h"

static wf_status_t fail_after_begin(const wf_saturn_render_ops_t *o,void *ctx,wf_status_t st){
    if(o->abort_frame)o->abort_frame(ctx);
    return st;
}

wf_status_t wf_saturn_render_submit(const wf_vdp1_frame_plan_t *p,const wf_vdp1_assets_t *a,const wf_saturn_render_ops_t *o,void *ctx){
    if(!p||!a||!o||!o->begin_frame||!o->upload_tile_16x16_4bpp||!o->upload_palette_xbgr444_16||!o->emit_sprite_16x16||!o->end_frame)return WF_BAD_STATE;
    wf_status_t st=wf_vdp1_plan_validate(p,a);if(st!=WF_OK)return st;
    st=o->begin_frame(ctx,p->tile_count,p->palette_count,p->cmd_count);if(st!=WF_OK)return st;
    for(unsigned i=0;i<p->tile_count;i++){
        const wf_vdp1_tile_upload_t*u=&p->tile_upload[i];
        st=o->upload_tile_16x16_4bpp(ctx,u->stream_slot,a->sprite_tiles+u->src_byte_offset,WF_VDP1_TILE_BYTES_16X16_4BPP);
        if(st!=WF_OK)return fail_after_begin(o,ctx,st);
    }
    for(unsigned i=0;i<p->palette_count;i++){
        const wf_vdp1_palette_upload_t*u=&p->palette_upload[i];
        st=o->upload_palette_xbgr444_16(ctx,u->cram_slot,a->palette_resources+u->src_byte_offset,WF_VDP1_PALETTE_BYTES);
        if(st!=WF_OK)return fail_after_begin(o,ctx,st);
    }
    for(unsigned i=0;i<p->cmd_count;i++){
        st=o->emit_sprite_16x16(ctx,&p->cmd[i]);
        if(st!=WF_OK)return fail_after_begin(o,ctx,st);
    }
    st=o->end_frame(ctx);
    if(st!=WF_OK){ if(o->abort_frame)o->abort_frame(ctx); return st; }
    return WF_OK;
}
