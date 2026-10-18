#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "wf_visual.h"
int main(void){
 wf_runtime_t rt;wf_runtime_init(&rt,0,0);wf_visual_state_t v;wf_visual_init(&v);
 wf_actor_raw_t *a=&rt.actors[0];wf_set_u8(a,WF_A_ACTIVE,1);wf_set_u8(a,0x33,1);wf_set_u8(a,WF_A_CAMERA_CLASS,0);
 wf_set_be16(a,WF_A_X,0x0200);wf_set_be16(a,WF_A_Y,0x0200);v.camera.scroll0_x=0x0140;v.camera.scroll1_x=0x0140;v.camera.scroll0_y=0x0200;v.camera.scroll1_y=0x0200;v.camera.scene_index=0;
 assert(wf_camera_track(&rt,&v.camera)==WF_OK);assert(v.camera.scroll0_x==0x0144); /* actor is +32 from normal X anchor */
 assert(wf_camera_clamp(&v.camera)==WF_OK);
 int16_t x,y;wf_world_to_screen(a,&v.camera,&x,&y);assert(x==0x00BC);assert(y==-4);
 puts("PASS test_visual_camera");return 0;
}
