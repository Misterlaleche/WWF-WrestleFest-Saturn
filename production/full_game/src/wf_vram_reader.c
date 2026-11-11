#include "wf_vram_reader.h"
#include <string.h>
void wf_vram_reader_hold(wf_vram_reader_t*r,wp1980_vram_cache_t*c){
 /* Moving an actor without changing its pose commonly reuses the exact
  * same immutable slots. Their reader bits already express this lease. */
 if(r->count==c->frame_count&&!memcmp(r->slot,c->frame_slot,r->count*sizeof r->slot[0]))return;
 for(unsigned i=0;i<r->count;i++)c->flags[r->slot[i]]&=(uint8_t)~WF_VRAM_READER;
 r->count=c->frame_count;
 for(unsigned i=0;i<r->count;i++){
  unsigned slot=c->frame_slot[i];r->slot[i]=(uint16_t)slot;
  c->flags[slot]|=WF_VRAM_READER;
 }
}
