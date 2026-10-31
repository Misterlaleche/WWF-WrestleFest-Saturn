#include "wf_pacer.h"
#include <string.h>
bool wf_pacer_init(wf_pacer_t*p,uint32_t now,uint32_t phase){
 if(!p||phase>=WF_ARCADE_THRESHOLD)return false;
 memset(p,0,sizeof *p);p->last_field=now;return wf_arcade_clock_init(&p->clock,phase);
}
bool wf_pacer_account(wf_pacer_t*p,uint32_t now){
 if(!p||p->clock.failed)return false;
 uint32_t elapsed=now-p->last_field;p->last_field=now;
 if(p->load_depth){
  if(UINT64_MAX-p->load_fields<elapsed){p->clock.failed=true;return false;}
  p->load_fields+=elapsed;
 }else while(elapsed--)if(!wf_arcade_clock_field(&p->clock))return false;
 return true;
}
bool wf_pacer_take(wf_pacer_t*p){
 if(!p)return false;
 if(p->load_depth){p->clock.failed=true;return false;}
 return wf_arcade_clock_take(&p->clock);
}
bool wf_pacer_load_begin(wf_pacer_t*p,uint32_t now){
 if(!wf_pacer_account(p,now))return false;
 if(p->load_depth==UINT32_MAX){p->clock.failed=true;return false;}
 ++p->load_depth;return true;
}
bool wf_pacer_load_end(wf_pacer_t*p,uint32_t now){
 if(!p||p->clock.failed)return false;
 if(!p->load_depth){p->clock.failed=true;return false;}
 if(!wf_pacer_account(p,now))return false;
 --p->load_depth;return true;
}
