#include "wf_hand_prompt.h"

const wf_hand_prompt_frame_meta_t wf_hand_prompt_frames[WF_HAND_PROMPT_POSES]={
 {WF_HAND_PROMPT_FINGER_UP,80,96,10,4,71,87,0,120,14,16,"79347a87029f42dd95f2e8d5ac5b511f07b523f2da9180f4263d22aa5c5e196b"},
 {WF_HAND_PROMPT_FINGER_DOWN,80,96,10,4,71,87,0,120,14,16,"3b931bf82a11af1f41b9dcaa679a4c9ccae02ca3ec49bcc0cc2525b9d9f9b7a4"}
};

bool wf_hand_prompt_select_pose(wf_hand_prompt_visual_t *visual,wf_hand_prompt_pose_t pose)
{
    if(!visual||(unsigned)pose>=WF_HAND_PROMPT_POSES)return false;
    visual->pose=pose;
    visual->visible=true;
    return true;
}

void wf_hand_prompt_hide(wf_hand_prompt_visual_t *visual)
{
    if(visual)visual->visible=false;
}
