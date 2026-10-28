#include <string.h>
#include "wf_r18_video.h"
int action7a_palette_oracle(uint8_t*memory,unsigned id){
 wf_r18_video_t v={0};memcpy(v.palette_used,memory+0x1c1600,16);memcpy(v.palette_map,memory+0x1c1610,256);
 wf_status_t st=wf_r18_palette_2b58(0,(uint16_t)id,&v);
 memcpy(memory+0x1c1600,v.palette_used,16);memcpy(memory+0x1c1610,v.palette_map,256);return st;
}
