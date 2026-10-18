#include "wf_camera.h"

static int16_t clamp4(int32_t v){ if(v>4)return 4; if(v<-4)return -4; return (int16_t)v; }
static int16_t s16(uint16_t v){return (int16_t)v;}
static int8_t s8(uint8_t v){return (int8_t)v;}

wf_status_t wf_camera_track(wf_runtime_t *rt,wf_camera_state_t *c){
    int32_t sx=0,sy=0; unsigned count=0;
    const int16_t anchor_x=(int16_t)(c->scroll0_x+0x00A0);
    const int16_t anchor_y=(int16_t)(c->scroll0_y-0x00F0);
    for(unsigned i=0;i<WF_ACTOR_SLOTS;i++){
        wf_actor_raw_t *a=&rt->actors[i]; if(!wf_u8(a,WF_A_ACTIVE)) continue;
        uint8_t cls=wf_u8(a,0x4B);
        if(cls==2u) return WF_OK;
        if(cls==3u) continue;
        if(cls==4u){
            int16_t dx=clamp4((int32_t)c->target_x-c->scroll0_x);
            int16_t dy=clamp4((int32_t)c->target_y-c->scroll0_y);
            c->scroll0_x=(int16_t)(c->scroll0_x+dx); c->scroll1_x=(int16_t)(c->scroll1_x+dx);
            c->scroll0_y=(int16_t)(c->scroll0_y+dy); c->scroll1_y=(int16_t)(c->scroll1_y+dy);
            return WF_OK;
        }
        if(cls==1u || ((rt->g.mode_0161&0x02u) && (wf_u8(a,0x33)&0x04u))){
            int16_t d7=(rt->g.mode_0161&0x02u)?0x0100:0x0140;
            sx=(int32_t)s16(wf_be16(a,WF_A_X))-anchor_x;
            sy=(int32_t)s16(wf_be16(a,WF_A_Y))+s16(wf_be16(a,WF_A_Z))-d7-anchor_y;
            count=1; break;
        }
        if(cls==0u && (wf_u8(a,0x33)&0x01u)){
            sx += (int32_t)s16(wf_be16(a,WF_A_X))-anchor_x;
            sy += (int32_t)s16(wf_be16(a,WF_A_Y))-anchor_y;
            count++;
        }
    }
    if(!count) return WF_OK;
    sx/= (int32_t)count; sy/= (int32_t)count;
    int16_t dx=clamp4(sx),dy=clamp4(sy);
    c->scroll0_x=(int16_t)(c->scroll0_x+dx); c->scroll1_x=(int16_t)(c->scroll1_x+dx);
    c->scroll0_y=(int16_t)(c->scroll0_y+dy); c->scroll1_y=(int16_t)(c->scroll1_y+dy);
    return WF_OK;
}

wf_status_t wf_camera_clamp(wf_camera_state_t *c){
    static const uint16_t rows[7][4]={{0x140,0x200,0x280,0x2C0},{0x140,0x200,0x280,0x2C0},{0x140,0x200,0x3C0,0x200},{0x140,0x200,0x780,0x700},{0x140,0x200,0x780,0x700},{0x140,0x200,0x280,0x2C0},{0x140,0x200,0x3C0,0x200}};
    if(c->scene_index>=7u) return WF_BLOCKED;
    int16_t xmin=(int16_t)rows[c->scene_index][0],ymin=(int16_t)rows[c->scene_index][1],xmax=(int16_t)rows[c->scene_index][2],ymax=(int16_t)rows[c->scene_index][3];
    if(c->scroll0_x<xmin) c->scroll0_x=xmin;
    if(c->scroll0_x>xmax) c->scroll0_x=xmax;
    if(c->scroll0_y<ymin) c->scroll0_y=ymin;
    if(c->scroll0_y>ymax) c->scroll0_y=ymax;
    if(c->scroll1_x<xmin) c->scroll1_x=xmin;
    if(c->scroll1_x>xmax) c->scroll1_x=xmax;
    if(c->scroll1_y<ymin) c->scroll1_y=ymin;
    if(c->scroll1_y>ymax) c->scroll1_y=ymax;
    return WF_OK;
}

void wf_world_to_screen(const wf_actor_raw_t *a,const wf_camera_state_t *c,int16_t *sx,int16_t *sy){
    int16_t dx=(int16_t)s8(wf_u8(a,0x19)); if(wf_u8(a,WF_A_FACING)&0x80u) dx=(int16_t)-dx;
    int16_t dy=(int16_t)s8(wf_u8(a,0x1B));
    *sx=(int16_t)(s16(wf_be16(a,WF_A_X))-c->scroll0_x+dx);
    *sy=(int16_t)(s16(wf_be16(a,WF_A_Y))+s16(wf_be16(a,WF_A_Z))-c->scroll0_y+dy);
}
