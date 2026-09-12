#include <assert.h>
#include <string.h>
#include "../../src/runtime/wf_hand_prompt.h"

int main(void)
{
 wf_hand_prompt_state_t s;
 wf_hand_prompt_output_t o;

 assert(WF_HAND_PROMPT_BYTES==7680u);
 assert(wf_hand_prompt_frames[0].origin_y==120);
 assert(wf_hand_prompt_frames[0].bbox_x0==10u&&wf_hand_prompt_frames[0].bbox_y0==4u);
 assert(wf_hand_prompt_frames[0].bbox_x1==71u&&wf_hand_prompt_frames[0].bbox_y1==87u);
 assert(strcmp(wf_hand_prompt_frames[0].idx8_sha256,"79347a87029f42dd95f2e8d5ac5b511f07b523f2da9180f4263d22aa5c5e196b")==0);
 assert(strcmp(wf_hand_prompt_frames[1].idx8_sha256,"3b931bf82a11af1f41b9dcaa679a4c9ccae02ca3ec49bcc0cc2525b9d9f9b7a4")==0);

 wf_hand_prompt_reset(&s);
 assert(wf_hand_prompt_step(&s,0u,&o)&&o.kind==WF_HAND_PROMPT_PASS_NONE);
 assert(wf_hand_prompt_step(&s,WF_HAND_PROMPT_FLAG_REQUEST,&o));
 assert(o.kind==WF_HAND_PROMPT_PASS_DRAW&&o.pose==WF_HAND_PROMPT_FINGER_UP&&o.write_count==78u);
 assert(o.writes[0].source_text_address==WF_HAND_PROMPT_TEXT_START);
 assert(o.writes[0].tile_code==0x0400u&&o.writes[0].palette==14u);
 assert(wf_hand_prompt_step(&s,WF_HAND_PROMPT_FLAG_REQUEST,&o));
 assert(o.kind==WF_HAND_PROMPT_PASS_DRAW&&o.pose==WF_HAND_PROMPT_FINGER_DOWN);
 assert(wf_hand_prompt_step(&s,0u,&o));
 assert(o.kind==WF_HAND_PROMPT_PASS_CLEAR&&o.pose==WF_HAND_PROMPT_FINGER_DOWN&&o.write_count==78u);
 assert(wf_hand_prompt_step(&s,0u,&o)&&o.kind==WF_HAND_PROMPT_PASS_NONE);
 assert(wf_hand_prompt_step(&s,WF_HAND_PROMPT_FLAG_REQUEST,&o));
 assert(o.pose==WF_HAND_PROMPT_FINGER_DOWN);

 wf_hand_prompt_reset(&s);
 s.previous_flags=WF_HAND_PROMPT_FLAG_SUPPRESS;
 assert(wf_hand_prompt_step(&s,WF_HAND_PROMPT_FLAG_REQUEST,&o)&&o.kind==WF_HAND_PROMPT_PASS_NONE);
 assert(s.previous_flags==WF_HAND_PROMPT_FLAG_SUPPRESS);
 assert(!wf_hand_prompt_step(0,0u,&o));
 assert(!wf_hand_prompt_step(&s,0u,0));
 return 0;
}
