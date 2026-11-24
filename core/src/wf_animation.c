#include <stddef.h>
#include "wf_animation.h"

static const uint16_t *dispatch_for(wf_anim_domain_t d,unsigned *n){
 switch(d){case WF_ANIM_MAIN:*n=13;return wf_anim_main_dispatch;case WF_ANIM_ACTION:*n=143;return wf_anim_action_dispatch;case WF_ANIM_STATE4:*n=43;return wf_anim_state4_dispatch;default:*n=0;return 0;}
}
static void load_entry(wf_actor_raw_t *a,wf_anim_view_t *v,uint16_t idx){
 const wf_anim_desc_t *d=&wf_anim_descs[v->desc_index];
 const wf_anim_entry_t *e=&wf_anim_entries[d->entry_start+idx];
 wf_set_be16(a,WF_A_ANIM_INDEX,idx);
 wf_set_be16(a,WF_A_ANIM_COUNTER,e->counter_raw);
 v->pose_raw=e->pose_raw;
 v->pose_flag15=(uint8_t)((e->pose_raw>>15)&1u);
 v->terminal=0;
}
int wf_anim_select(wf_actor_raw_t *a,wf_anim_domain_t dom,uint8_t id,wf_anim_view_t *v){
 unsigned n=0; const uint16_t *t=dispatch_for(dom,&n); if(!t||id>=n)return 0;
 uint16_t di=t[id]; if(di>=wf_anim_desc_count)return 0;
 v->desc_index=di; v->pose_raw=0; v->pose_flag15=0; v->terminal=0;
 const wf_anim_desc_t *d=&wf_anim_descs[di];
 wf_set_be16(a,WF_A_ANIM_INDEX,0);
 if(d->kind==0 || d->entry_count==0){ wf_set_be16(a,WF_A_ANIM_COUNTER,0); return 1; }
 if(d->stored_entries<d->entry_count)return 0;
 load_entry(a,v,0); return 1;
}
int wf_anim_tick(wf_actor_raw_t *a,wf_anim_view_t *v){
 if(v->desc_index>=wf_anim_desc_count)return 0;
 const wf_anim_desc_t *d=&wf_anim_descs[v->desc_index];
 if(d->kind==0 || d->entry_count==0)return 1; /* callback-owned */
 uint16_t c=wf_be16(a,WF_A_ANIM_COUNTER);
 if(c==0xFF00u)return 1; /* hard hold sentinel */
 c=(uint16_t)(c-1u); wf_set_be16(a,WF_A_ANIM_COUNTER,c);
 if(c!=0xFFFFu)return 1; /* N+1 semantics */
 uint16_t i=(uint16_t)(wf_be16(a,WF_A_ANIM_INDEX)+1u);
 if(i>=d->entry_count){
   if(d->kind==2u){load_entry(a,v,0);return 1;}
   wf_set_be16(a,WF_A_ANIM_INDEX,0x00FEu); v->terminal=1; return 1;
 }
 load_entry(a,v,i); return 1;
}
uint16_t wf_anim_current_pose(const wf_actor_raw_t *a,const wf_anim_view_t *v){(void)a;return (uint16_t)(v->pose_raw&0x7FFFu);}
