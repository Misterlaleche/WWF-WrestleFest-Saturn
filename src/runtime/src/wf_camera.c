#include "wf_camera.h"

static int16_t clamp4(int32_t v){ if(v>4)return 4; if(v<-4)return -4; return (int16_t)v; }
static int16_t s16(uint16_t v){return (int16_t)v;}
static int8_t s8(uint8_t v){return (int8_t)v;}

wf_status_t wf_camera_track(wf_runtime_t *rt,wf_camera_state_t *c){
    wf_r17_m68k_shadow_t *r=&rt->m68k_regs;
    uint16_t sx=0,sy=0,count=0;
    uint16_t ax=(uint16_t)(c->scroll0_x+0xa0),ay=(uint16_t)(c->scroll0_y-0xf0);
    wf_r17_m68k_move_w(r,0,0);wf_r17_m68k_move_l(r,1,0);wf_r17_m68k_move_l(r,2,0);
    wf_r17_m68k_move_w(r,3,ax);wf_r17_m68k_move_w(r,4,ay);wf_r17_m68k_move_w(r,5,0);
    int target=0;
    for(unsigned i=0;i<WF_ACTOR_SLOTS;i++){
        wf_r17_m68k_move_w(r,0,(uint16_t)(i+1));
        wf_actor_raw_t *a=&rt->actors[i];if((wf_u8(a,WF_A_ACTIVE)&0x80u)==0)continue;
        uint8_t cls=wf_u8(a,WF_A_CAMERA_CLASS);
        if(cls==2)return WF_OK;
        if(cls==3)continue;
        if(cls==4){sx=(uint16_t)(c->target_x-c->scroll0_x);sy=(uint16_t)(c->target_y-c->scroll0_y);target=1;break;}
        if(cls!=1){
            if((wf_u8(a,WF_A_FLAGS33)&1u)==0)continue;
            count++;wf_r17_m68k_move_w(r,5,count);
            uint16_t delta=(uint16_t)(wf_be16(a,WF_A_X)-ax);wf_r17_m68k_move_w(r,6,delta);sx=(uint16_t)(sx+delta);
            delta=(uint16_t)(wf_be16(a,WF_A_Y)-ay);wf_r17_m68k_move_w(r,6,delta);sy=(uint16_t)(sy+delta);
            wf_r17_m68k_move_w(r,1,sx);wf_r17_m68k_move_w(r,2,sy);
            if((rt->g.mode_0161&2u)==0||(wf_u8(a,WF_A_FLAGS33)&4u)==0)continue;
        }
        uint16_t height=(rt->g.mode_0161&2u)?0x100:0x140;
        wf_r17_m68k_move_w(r,7,height);count=1;wf_r17_m68k_moveq(r,5,1);
        sx=(uint16_t)(wf_be16(a,WF_A_X)-ax);sy=(uint16_t)(wf_be16(a,WF_A_Y)+wf_be16(a,WF_A_Z)-height-ay);break;
    }
    if(!target){
        if(!count)return WF_OK;
        /* EXT.L/DIVS.W retains signed remainder in the upper WORD. */
        int32_t x=(int16_t)sx,y=(int16_t)sy;
        wf_r17_m68k_move_l(r,1,((uint32_t)(uint16_t)(x%(int16_t)count)<<16)|(uint16_t)(x/(int16_t)count));
        wf_r17_m68k_move_l(r,2,((uint32_t)(uint16_t)(y%(int16_t)count)<<16)|(uint16_t)(y/(int16_t)count));
        sx=(uint16_t)r->d[1];sy=(uint16_t)r->d[2];
    }else{wf_r17_m68k_move_w(r,1,sx);wf_r17_m68k_move_w(r,2,sy);}
    int16_t dx=clamp4((int16_t)sx),dy=clamp4((int16_t)sy);
    wf_r17_m68k_move_w(r,1,(uint16_t)dx);wf_r17_m68k_move_w(r,2,(uint16_t)dy);
    c->scroll0_x=(int16_t)(c->scroll0_x+dx);c->scroll1_x=(int16_t)(c->scroll1_x+dx);
    c->scroll0_y=(int16_t)(c->scroll0_y+dy);c->scroll1_y=(int16_t)(c->scroll1_y+dy);
    return WF_OK;
}

wf_status_t wf_camera_clamp(wf_camera_state_t *c){
    static const uint16_t rows[7][4]={{0x140,0x200,0x280,0x2C0},{0x140,0x200,0x280,0x2C0},{0x140,0x200,0x3C0,0x200},{0x140,0x200,0x780,0x700},{0x140,0x200,0x780,0x700},{0x140,0x200,0x280,0x2C0},{0x140,0x200,0x3C0,0x200}};
    if(c->scene_index>=7u) return WF_BLOCKED;
    uint16_t xmin=rows[c->scene_index][0],ymin=rows[c->scene_index][1],xmax=rows[c->scene_index][2],ymax=rows[c->scene_index][3];
    /* $2983C uses unsigned BCC/BCS on channel A. Only a clamp writes
     * both channels; an in-range A leaves B untouched, even at its edge. */
    if((uint16_t)c->scroll0_x<xmin)c->scroll0_x=c->scroll1_x=(int16_t)xmin;
    else if((uint16_t)c->scroll0_x>=xmax)c->scroll0_x=c->scroll1_x=(int16_t)xmax;
    if((uint16_t)c->scroll0_y<ymin)c->scroll0_y=c->scroll1_y=(int16_t)ymin;
    else if((uint16_t)c->scroll0_y>=ymax)c->scroll0_y=c->scroll1_y=(int16_t)ymax;
    return WF_OK;
}

void wf_world_to_screen(const wf_actor_raw_t *a,const wf_camera_state_t *c,int16_t *sx,int16_t *sy){
    int16_t dx=(int16_t)s8(wf_u8(a,0x19)); if(wf_u8(a,WF_A_FACING)&0x80u) dx=(int16_t)-dx;
    int16_t dy=(int16_t)s8(wf_u8(a,0x1B));
    *sx=(int16_t)(s16(wf_be16(a,WF_A_X))-c->scroll0_x+dx);
    *sy=(int16_t)(s16(wf_be16(a,WF_A_Y))+s16(wf_be16(a,WF_A_Z))-c->scroll0_y+dy);
}
