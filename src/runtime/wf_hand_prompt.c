#include "wf_hand_prompt.h"
#include <string.h>

const wf_hand_prompt_frame_meta_t wf_hand_prompt_frames[WF_HAND_PROMPT_POSES]={
 {WF_HAND_PROMPT_FINGER_UP,80,96,10,4,71,87,0,120,14,16,"79347a87029f42dd95f2e8d5ac5b511f07b523f2da9180f4263d22aa5c5e196b"},
 {WF_HAND_PROMPT_FINGER_DOWN,80,96,10,4,71,87,0,120,14,16,"3b931bf82a11af1f41b9dcaa679a4c9ccae02ca3ec49bcc0cc2525b9d9f9b7a4"}
};

static const uint16_t rows[13][2]={
 {0x0700,0x0707},{0x070E,0x0715},{0x031C,0x031F},{0x0522,0x0527},{0x052C,0x0531},
 {0x0836,0x083E},{0x0846,0x084E},{0x0856,0x085E},{0x0766,0x076D},{0x0774,0x077B},
 {0x0782,0x0789},{0x0590,0x0596},{0x0195,0x019B}
};
static const int16_t corr[13]={0,8,-4,0,-8,0,0,4,-4,0,0,-232,0};

static void emit(wf_hand_prompt_pose_t pose,bool clear,wf_hand_prompt_output_t*out)
{
 uint32_t a=WF_HAND_PROMPT_TEXT_START;
 uint16_t n=0,r;
 for(r=0;r<13u;r++){
  uint16_t d=rows[r][pose==WF_HAND_PROMPT_FINGER_DOWN?1u:0u];
  uint8_t count=(uint8_t)(d>>8),lo=(uint8_t)d,i;
  for(i=0;i<count;i++){
   out->writes[n].source_text_address=a+(uint32_t)i*4u;
   out->writes[n].tile_code=clear?0u:(uint16_t)(0x0400u+lo+i);
   out->writes[n].palette=clear?0u:WF_HAND_PROMPT_TEXT_PALETTE;
   out->writes[n].clear=clear;
   n++;
  }
  a+=0x100u;
  a=(uint32_t)((int32_t)a+corr[r]);
 }
 out->write_count=n;
}

void wf_hand_prompt_reset(wf_hand_prompt_state_t*s)
{
 if(s)memset(s,0,sizeof(*s));
}

bool wf_hand_prompt_step(wf_hand_prompt_state_t*s,uint8_t current,wf_hand_prompt_output_t*out)
{
 bool clear=false;
 if(!s||!out)return false;
 memset(out,0,sizeof(*out));
 out->pose=(wf_hand_prompt_pose_t)(s->pose_word&1u);
 if(s->previous_flags&WF_HAND_PROMPT_FLAG_SUPPRESS)return true;
 if(!(current&WF_HAND_PROMPT_FLAG_REQUEST)){
  if(!(s->previous_flags&WF_HAND_PROMPT_FLAG_REQUEST))return true;
  current=(uint8_t)(current|WF_HAND_PROMPT_FLAG_CLEAR);
  clear=true;
 }
 s->cadence_counter++;
 if(s->cadence_counter>=2u){s->cadence_counter=0u;s->pose_word^=1u;}
 out->pose=(wf_hand_prompt_pose_t)(s->pose_word&1u);
 out->kind=clear?WF_HAND_PROMPT_PASS_CLEAR:WF_HAND_PROMPT_PASS_DRAW;
 emit(out->pose,clear,out);
 s->previous_flags=current;
 return true;
}
