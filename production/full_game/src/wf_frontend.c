#include "wf_frontend.h"
#include <string.h>
#include "../vendor/frontend_test03/nav_tables.h"
void wf_frontend_init(wf_frontend_t*f){memset(f,0,sizeof *f);f->state=WF_FRONTEND_MODE;f->scroll=320;}
void wf_frontend_roster_ready(wf_frontend_t*f){if(f->state==WF_FRONTEND_LOAD_ROSTER)f->state=WF_FRONTEND_ROSTER;}
static void scroll(wf_frontend_t*f,wf_frontend_effect_t*e){
 if(f->scroll_left==1){f->scroll_left=0;f->page=f->to_right?1:0;f->state=WF_FRONTEND_ROSTER;return;}
 f->scroll=f->to_right?f->scroll+8:f->scroll-8;e->scroll=true;
 --f->scroll_left; /* TEST03 waits once more in its outer loop after step26. */
}
bool wf_frontend_tick(wf_frontend_t*f,unsigned d,bool confirm,wf_frontend_effect_t*e){
 static const uint8_t pulse[10]={1,2,3,4,5,4,3,2,1,0};
 if(!f||!e)return false;
 memset(e,0,sizeof *e);
 if(f->state==WF_FRONTEND_MODE){
  if(confirm){f->state=WF_FRONTEND_LOAD_ROSTER;return true;}
  bool changed=false;
  if(f->royal&&(d==1||d==5||d==9)){f->royal=false;changed=true;}
  else if(!f->royal&&(d==2||d==6||d==10)){f->royal=true;changed=true;}
  if(changed){e->palette=true;e->family=!f->royal;f->phase_index=f->dwell=0;}
  else if(++f->dwell>=5){f->dwell=0;e->palette=true;e->family=f->royal;e->phase=pulse[f->phase_index];f->phase_index=(f->phase_index+1)%10;}
  return true;
 }
 if(f->state==WF_FRONTEND_SCROLL){scroll(f,e);return true;}
 if(f->state!=WF_FRONTEND_ROSTER)return false;
 if(confirm){e->confirm=true;f->state=WF_FRONTEND_DONE;return true;}
 if(d>10){f->state=WF_FRONTEND_ERROR;return false;}
 if(d==f->previous_direction)return true;
 f->previous_direction=d;if(!d)return true;
 unsigned dest=f->slot;
 if(f->page==0){if(f->slot<8)dest=nav_left[f->slot][d];}
 else dest=(f->royal?nav_right_royal:nav_right_tag)[f->slot][d];
 if(dest<10&&dest!=f->slot){
  unsigned old=f->slot;e->cursor=true;e->old_slot=old;f->slot=dest;
  bool right=(f->page==0&&((old==2&&dest==3)||(old==7&&dest==8)));
  bool left=(f->page==1&&((old==2&&dest==1)||(old==7&&dest==6)));
  if(right||left){f->state=WF_FRONTEND_SCROLL;f->to_right=right;f->scroll_left=27;scroll(f,e);}
 }
 return true;
}
