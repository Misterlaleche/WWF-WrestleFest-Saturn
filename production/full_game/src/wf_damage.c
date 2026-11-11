#include "wf_damage.h"
#include <string.h>
static void empty(uint32_t *bands){
 memset(bands,0,WF_DAMAGE_BANDS*sizeof *bands);
}
void wf_damage_init(wf_damage_t*d){
 empty(d->history[0]);empty(d->history[1]);empty(d->current);d->full_clears=2;
}
unsigned wf_damage_begin(wf_damage_t*d,wf_damage_rect_t out[WF_DAMAGE_RECTS_MAX]){
 empty(d->current);
 if(d->full_clears){out[0]=(wf_damage_rect_t){0,0,WF_DAMAGE_WIDTH,WF_DAMAGE_HEIGHT};return 1;}
 unsigned count=0;
 for(unsigned i=0;i<WF_DAMAGE_BANDS;i++){
  unsigned bits=d->history[0][i]|d->history[1][i],cell=0;
  while(bits){
   while(!(bits&1u)){bits>>=1;++cell;}
   unsigned first=cell;
   do{bits>>=1;++cell;}while(bits&1u);
   unsigned left=first*16u,width=(cell-first)*16u;
   if(count&&out[count-1].x==left&&out[count-1].width==width&&
      out[count-1].y+out[count-1].height==i*16u)out[count-1].height+=16;
   else out[count++]=(wf_damage_rect_t){left,i*16u,width,16};
  }
 }
 return count;
}
void wf_damage_add(wf_damage_t*d,int16_t x,int16_t y,uint16_t width,uint16_t height){
 static const uint32_t low_bits[21]={0,1,3,7,15,31,63,127,255,511,1023,2047,4095,8191,16383,32767,65535,131071,262143,524287,1048575};
 if(!width||!height||x>=WF_DAMAGE_WIDTH||y>=WF_DAMAGE_HEIGHT)return;
 int right=(int)x+width-1,bottom=(int)y+height-1;
 if(right<0||bottom<0)return;
 unsigned left=x<0?0u:(unsigned)x,top=y<0?0u:(unsigned)y;
 if(right>=WF_DAMAGE_WIDTH)right=WF_DAMAGE_WIDTH-1;
 if(bottom>=WF_DAMAGE_HEIGHT)bottom=WF_DAMAGE_HEIGHT-1;
 unsigned mask=low_bits[(unsigned)right/16+1]^low_bits[left/16];
 for(unsigned i=top/16;i<=(unsigned)bottom/16;i++)d->current[i]|=mask;
}
void wf_damage_commit(wf_damage_t*d){
 memcpy(d->history[0],d->history[1],sizeof d->current);
 memcpy(d->history[1],d->current,sizeof d->current);
 if(d->full_clears)--d->full_clears;
}
