#include <assert.h>
#include <string.h>
#include "../../src/runtime/wf_hand_prompt.h"

int main(void)
{
    wf_hand_prompt_visual_t v={0};
    assert(WF_HAND_PROMPT_BYTES==7680u);
    assert(wf_hand_prompt_frames[0].origin_y==120);
    assert(wf_hand_prompt_frames[0].bbox_x0==10&&wf_hand_prompt_frames[0].bbox_y0==4);
    assert(wf_hand_prompt_frames[0].bbox_x1==71&&wf_hand_prompt_frames[0].bbox_y1==87);
    assert(strcmp(wf_hand_prompt_frames[0].idx8_sha256,"79347a87029f42dd95f2e8d5ac5b511f07b523f2da9180f4263d22aa5c5e196b")==0);
    assert(strcmp(wf_hand_prompt_frames[1].idx8_sha256,"3b931bf82a11af1f41b9dcaa679a4c9ccae02ca3ec49bcc0cc2525b9d9f9b7a4")==0);
    assert(wf_hand_prompt_select_pose(&v,WF_HAND_PROMPT_FINGER_DOWN));
    assert(v.visible&&v.pose==WF_HAND_PROMPT_FINGER_DOWN);
    wf_hand_prompt_hide(&v);
    assert(!v.visible);
    assert(!wf_hand_prompt_select_pose(&v,(wf_hand_prompt_pose_t)2));
    return 0;
}
