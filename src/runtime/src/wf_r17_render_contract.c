#include "wf_r17_contract.h"

static int16_t sx8(uint8_t v){ return (int16_t)(int8_t)v; }
static int signed_y_desc(const wf_actor_raw_t *a,const wf_actor_raw_t *b){
    /* ROM $2962/$2966 uses MOVE.W/CMP.W followed by BGE: signed 16-bit compare. */
    return (int16_t)wf_be16(a,WF_A_Y) < (int16_t)wf_be16(b,WF_A_Y);
}

wf_r17_tri_t wf_r17_visibility_2188_exact(wf_actor_raw_t *a,void *user){
    int16_t x,y;(void)user;if(!a)return WF_R17_UNKNOWN;x=(int16_t)wf_be16(a,WF_A_SCREEN_X);y=(int16_t)wf_be16(a,WF_A_SCREEN_Y);
    return (x>=-63 && x<383 && y>=-63 && y<319)?WF_R17_YES:WF_R17_NO;
}

void wf_r17_render_queue_clear(wf_r17_render_queue_t *q){ unsigned k,i; if(!q)return; for(k=0;k<WF_R17_RENDER_LISTS;k++){q->count[k]=0u;for(i=0;i<WF_R17_RENDER_LIST_CAP;i++)q->list[k][i]=0;} }

wf_status_t wf_r17_world_to_screen_247c(wf_actor_raw_t *a,uint16_t sx,uint16_t sy){
    int16_t xo,yo;
    uint16_t x,y,z;
    if(!a) return WF_BAD_STATE;
    xo=sx8(wf_u8(a,WF_A_X_CORR8));
    yo=sx8(wf_u8(a,WF_A_Y_CORR8));
    if((wf_u8(a,WF_A_FACING)&0x80u)!=0u) xo=(int16_t)-xo;
    x=wf_be16(a,WF_A_X); y=wf_be16(a,WF_A_Y); z=wf_be16(a,WF_A_Z);
    wf_set_be16(a,WF_A_SCREEN_X,(uint16_t)(x-sx+(uint16_t)xo));
    wf_set_be16(a,WF_A_SCREEN_Y,(uint16_t)(y+z-sy+(uint16_t)yo));
    return WF_OK;
}

wf_status_t wf_r17_queue_27b8(wf_r17_render_ctx_t *ctx,wf_actor_raw_t *a){
    uint16_t visual,klass,n;
    wf_r17_tri_t vis;
    if(!ctx||!ctx->queue||!a) return WF_BAD_STATE;
    if((wf_u8(a,WF_A_ACTIVE)&0x80u)==0u) return WF_OK;
    visual=(uint16_t)(wf_be16(a,WF_A_MIRROR)&0x7fffu);
    if(visual==0x7fffu) return WF_OK;
    if(wf_u8(a,WF_A_CHARACTER+1u)<0x4eu){
        if(!ctx->visibility_2188) return WF_BLOCKED;
        vis=ctx->visibility_2188(a,ctx->visibility_user);
        if(vis==WF_R17_UNKNOWN) return WF_BLOCKED;
        if(vis==WF_R17_NO) return WF_OK; /* $2188 carry set */
    }
    klass=wf_be16(a,WF_A_RENDER_LIST);
    if(klass>=WF_R17_RENDER_LISTS) return WF_BAD_STATE;
    n=ctx->queue->count[klass];
    if(n>=WF_R17_RENDER_LIST_CAP) return WF_BAD_STATE;
    ctx->queue->list[klass][n]=a;
    ctx->queue->count[klass]=(uint16_t)(n+1u);
    return WF_OK;
}

static void stable_desc_y(wf_actor_raw_t **v,uint16_t n){
    uint16_t i,j;
    for(i=1u;i<n;i++){
        wf_actor_raw_t *key=v[i]; j=i;
        while(j>0u && signed_y_desc(v[j-1u],key)){v[j]=v[j-1u];j--;}
        v[j]=key;
    }
}
size_t wf_r17_emit_actor_order_2836(wf_r17_render_queue_t *q,wf_actor_raw_t **out,size_t cap){
    static const uint8_t order[6]={5,4,3,2,1,0};
    static const uint8_t sorted[6]={1,0,1,0,1,1}; /* exact: lists 0,2,4,5 sort; 1,3 insertion order */
    size_t used=0; unsigned oi;
    if(!q||!out) return 0u;
    for(oi=0;oi<6u;oi++){
        unsigned k=order[oi];uint16_t i,n=q->count[k];
        if(sorted[k]) stable_desc_y(q->list[k],n);
        for(i=0;i<n;i++){if(used>=cap)return used;out[used++]=q->list[k][i];}
        q->count[k]=0u; /* $28FA clears count before consumption */
    }
    return used;
}
