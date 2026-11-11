#ifndef WF_SCENE_BACKGROUND_H
#define WF_SCENE_BACKGROUND_H
#include "wf_r18_video.h"

/* Capture while the combat ring and its resident coordinates still exist.
 * Its original reader resolves both scrolling and animation overlays back
 * to arcade tile IDs; untouched cells retain their existing board bytes.
 * No owner, camera, queue or clock advances. A read failure is terminal and
 * may leave a completed prefix; do not pack or resume that partial image. */
static inline wf_status_t wf_scene_background_capture(wf_r18_video_t *video){
 if(!video||!video->ring_cell)return WF_BAD_STATE;
 for(unsigned layer=0;layer<2;layer++){
  unsigned width=layer?2u:4u;
  uint8_t *bytes=video->background+(layer?0x2000u:0u);
  for(unsigned cell=0;cell<1024;cell++,bytes+=width)
   if(video->ring_cell(layer,cell,bytes,WF_R18_BACKGROUND_READ))return WF_BLOCKED;
 }
 return WF_OK;
}
#endif
