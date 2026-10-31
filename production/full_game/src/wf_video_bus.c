#include "wf_r18_video.h"

/* Keep the complete original transaction path for batches, map/ring stores,
 * odd/partial accesses, padding and errors. Only an aligned visible palette
 * word can bypass its repeated map/validation/apply dispatch. */
extern wf_status_t wf_video_batch_reference(wf_runtime_t*,const wf_r17_fg0_write_t*,size_t,void*);
wf_status_t wf_r18_video_batch(wf_runtime_t*r,const wf_r17_fg0_write_t*w,size_t n,void*u){
 if(u&&w&&n==1&&w->width==2&&!(w->addr&1u)&&(w->addr&127u)<32u){
  wf_r18_video_t*v=u;unsigned role;uint8_t*p;
  switch(w->addr&0xfffff800u){
  case 0x180000u:role=WF_R18_PALETTE_TEXT;p=v->palettes;break;
  case 0x182000u:role=WF_R18_PALETTE_SPRITES;p=v->palettes+0x2000;break;
  case 0x186000u:role=WF_R18_PALETTE_BG;p=v->bg_palette;break;
  case 0x188000u:role=WF_R18_PALETTE_FG;p=v->fg_palette;break;
  default:return wf_video_batch_reference(r,w,n,u);
  }
  p+=w->addr&0x7ffu;uint8_t hi=(uint8_t)(w->value>>8),lo=(uint8_t)w->value;
  if(p[0]!=hi||p[1]!=lo){p[0]=hi;p[1]=lo;v->palette_dirty[role]|=(uint16_t)(1u<<((w->addr>>7)&15u));}
  return WF_OK;
 }
 return wf_video_batch_reference(r,w,n,u);
}
