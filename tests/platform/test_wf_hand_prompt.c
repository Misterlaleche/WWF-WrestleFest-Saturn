#include <assert.h>
#include <string.h>
#include "../../src/runtime/wf_hand_prompt.h"

int main(void)
{
 wf_hand_prompt_state_t s;
 wf_hand_prompt_output_t o;
 uint8_t current=0u,previous=0u;

 assert(WF_HAND_PROMPT_BYTES==7680u);
 assert(wf_hand_prompt_frames[0].origin_y==120);
 assert(wf_hand_prompt_frames[0].bbox_x0==10u&&wf_hand_prompt_frames[0].bbox_y0==4u);
 assert(wf_hand_prompt_frames[0].bbox_x1==71u&&wf_hand_prompt_frames[0].bbox_y1==94u);
 assert(strcmp(wf_hand_prompt_frames[0].idx8_sha256,"352072fb1ab6b2be33a42b469572189b9202ed36578b14289002261d8316bf57")==0);
 assert(strcmp(wf_hand_prompt_frames[1].idx8_sha256,"36ca81e2ac88ddb113000775003e9f0db0e8f087f9b0bf75660ec73d01527a02")==0);

 wf_hand_prompt_reset(&s);
 assert(wf_hand_prompt_step(&s,&current,&previous,&o)&&o.kind==WF_HAND_PROMPT_PASS_NONE);

 current=WF_HAND_PROMPT_FLAG_REQUEST;
 assert(wf_hand_prompt_step(&s,&current,&previous,&o));
 assert(o.kind==WF_HAND_PROMPT_PASS_DRAW&&o.pose==WF_HAND_PROMPT_FINGER_UP&&o.write_count==78u);
 assert(current==0u&&previous==WF_HAND_PROMPT_FLAG_REQUEST);
 assert(o.saturn_nbg==2u&&o.saturn_priority==4u);
 assert(o.writes[0].source_text_address==WF_HAND_PROMPT_TEXT_START);
 assert(o.writes[0].tile_code==0x0400u&&o.writes[0].palette==14u);
 assert(o.writes[0].raster_x==16&&o.writes[0].raster_y==120);
 assert(o.writes[77].raster_x==56&&o.writes[77].raster_y==208);

 current=WF_HAND_PROMPT_FLAG_REQUEST;
 assert(wf_hand_prompt_step(&s,&current,&previous,&o));
 assert(o.kind==WF_HAND_PROMPT_PASS_DRAW&&o.pose==WF_HAND_PROMPT_FINGER_DOWN);

 assert(wf_hand_prompt_step(&s,&current,&previous,&o));
 assert(o.kind==WF_HAND_PROMPT_PASS_CLEAR&&o.pose==WF_HAND_PROMPT_FINGER_DOWN);
 assert((previous&WF_HAND_PROMPT_FLAG_CLEAR)!=0u&&current==0u);

 assert(wf_hand_prompt_step(&s,&current,&previous,&o)&&o.kind==WF_HAND_PROMPT_PASS_NONE);

 previous=WF_HAND_PROMPT_FLAG_SUPPRESS;
 current=WF_HAND_PROMPT_FLAG_REQUEST;
 assert(wf_hand_prompt_step(&s,&current,&previous,&o)&&o.kind==WF_HAND_PROMPT_PASS_NONE);
 assert(current==WF_HAND_PROMPT_FLAG_REQUEST&&previous==WF_HAND_PROMPT_FLAG_SUPPRESS);

 assert(!wf_hand_prompt_step(0,&current,&previous,&o));
 assert(!wf_hand_prompt_step(&s,0,&previous,&o));
 assert(!wf_hand_prompt_step(&s,&current,0,&o));
 assert(!wf_hand_prompt_step(&s,&current,&previous,0));
 return 0;
}
