#include "wf_hand_prompt.h"
#include <string.h>

const wf_hand_prompt_frame_meta_t wf_hand_prompt_frames[WF_HAND_PROMPT_POSES]={
 {WF_HAND_PROMPT_FINGER_UP,80,96,10,4,71,94,0,120,14,16,"352072fb1ab6b2be33a42b469572189b9202ed36578b14289002261d8316bf57"},
 {WF_HAND_PROMPT_FINGER_DOWN,80,96,10,4,71,94,0,120,14,16,"36ca81e2ac88ddb113000775003e9f0db0e8f087f9b0bf75660ec73d01527a02"}
};

static const uint16_t rows[WF_HAND_PROMPT_ROW_COUNT][2]={
 {0x0700,0x0707},{0x070E,0x0715},{0x031C,0x031F},{0x0522,0x0527},{0x052C,0x0531},
 {0x0836,0x083E},{0x0846,0x084E},{0x0856,0x085E},{0x0766,0x076D},{0x0774,0x077B},
 {0x0782,0x0789},{0x0590,0x0596},{0x0195,0x019B}
};
static const int16_t corr[WF_HAND_PROMPT_ROW_COUNT]={0,8,-4,0,-8,0,0,4,-4,0,0,-232,0};

void wf_hand_prompt_text_address_to_raster(uint32_t address,int16_t*x,int16_t*y)
{
 uint32_t cell=(address-WF_HAND_PROMPT_TEXT_BASE)/4u;
 uint32_t col=cell%WF_HAND_PROMPT_TEXT_COLUMNS;
 uint32_t row=cell/WF_HAND_PROMPT_TEXT_COLUMNS;
 if(x)*x=(int16_t)(col*WF_HAND_PROMPT_CELL_PIXELS);
 if(y)*y=(int16_t)((int32_t)(row*WF_HAND_PROMPT_CELL_PIXELS)+WF_HAND_PROMPT_RASTER_Y_BIAS);
}

static void emit(wf_hand_prompt_pose_t pose,bool clear,wf_hand_prompt_output_t*out)
{
 uint32_t a=WF_HAND_PROMPT_TEXT_START;
 uint16_t n=0u,r;
 for(r=0u;r<WF_HAND_PROMPT_ROW_COUNT;r++){
  uint16_t d=rows[r][pose==WF_HAND_PROMPT_FINGER_DOWN?1u:0u];
  uint8_t count=(uint8_t)(d>>8),lo=(uint8_t)d,i;
  for(i=0u;i<count;i++){
   wf_hand_prompt_tile_write_t*w=&out->writes[n++];
   w->source_text_address=a+(uint32_t)i*4u;
   w->tile_code=clear?0u:(uint16_t)(0x0400u+lo+i);
   w->palette=clear?0u:WF_HAND_PROMPT_TEXT_PALETTE;
   w->clear=clear;
   wf_hand_prompt_text_address_to_raster(w->source_text_address,&w->raster_x,&w->raster_y);
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

bool wf_hand_prompt_step(wf_hand_prompt_state_t*s,uint8_t*current,uint8_t*previous,wf_hand_prompt_output_t*out)
{
 uint8_t flags;
 bool clear=false;
 if(!s||!current||!previous||!out)return false;
 memset(out,0,sizeof(*out));
 out->pose=(wf_hand_prompt_pose_t)(s->pose_word&1u);
 out->saturn_nbg=WF_HAND_PROMPT_SATURN_NBG;
 out->saturn_priority=WF_HAND_PROMPT_SATURN_EQUIV_PRIORITY;
 flags=*current;
 if((*previous&WF_HAND_PROMPT_FLAG_SUPPRESS)!=0u)return true;
 if((flags&WF_HAND_PROMPT_FLAG_REQUEST)==0u){
  if((*previous&WF_HAND_PROMPT_FLAG_REQUEST)==0u)return true;
  flags=(uint8_t)(flags|WF_HAND_PROMPT_FLAG_CLEAR);
  *current=flags;
  clear=true;
 }
 s->cadence_counter=(uint16_t)(s->cadence_counter+1u);
 if(s->cadence_counter>=2u){s->cadence_counter=0u;s->pose_word^=1u;}
 out->pose=(wf_hand_prompt_pose_t)(s->pose_word&1u);
 out->kind=clear?WF_HAND_PROMPT_PASS_CLEAR:WF_HAND_PROMPT_PASS_DRAW;
 emit(out->pose,clear,out);
 *previous=*current;
 *current=0u;
 return true;
}
