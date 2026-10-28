/* The resident-slot remap runs every rendered frame. Verify its output and
 * count clears of its 512-entry lookup across two generation wraps. */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "wf_vdp1_plan.h"

static size_t lookup_cleared_bytes;
static void *counted_memset(void *dst,int value,size_t bytes);
#define memset counted_memset
#include "../../src/wf_vdp1_plan.c"
#undef memset

static void *counted_memset(void *dst,int value,size_t bytes){
 /* The only 1 KiB clear inside the included implementation is lookup. */
 if(bytes==512u*sizeof(uint16_t))lookup_cleared_bytes+=bytes;
 return memset(dst,value,bytes);
}

int main(void){
 const uint16_t resident[3]={0,0,233};
 const uint8_t resident_bits[1]={7};
 wf_vdp1_frame_plan_t frame;
 for(unsigned tick=0;tick<260;tick++){
  uint16_t changing=(uint16_t)(1000u+tick);
  memset(&frame,0,sizeof frame);
  frame.tile_count=3;frame.cmd_count=4;
  frame.tile_upload[0].source_tile=233;
  frame.tile_upload[1].source_tile=0;
  frame.tile_upload[2].source_tile=changing;
  frame.cmd[0].stream_tile=0;frame.cmd[1].stream_tile=1;
  frame.cmd[2].stream_tile=2;frame.cmd[3].stream_tile=0;
  assert(wf_vdp1_plan_retain_tile_slots(&frame,resident,resident_bits,3)==WF_OK);
  assert(frame.tile_upload[0].source_tile==0);
  assert(frame.tile_upload[1].source_tile==changing);
  assert(frame.tile_upload[2].source_tile==233);
  for(unsigned slot=0;slot<3;slot++)assert(frame.tile_upload[slot].stream_slot==slot);
  assert(frame.cmd[0].stream_tile==2&&frame.cmd[1].stream_tile==0&&
         frame.cmd[2].stream_tile==1&&frame.cmd[3].stream_tile==2);
 }
 assert(lookup_cleared_bytes<=2u*512u*sizeof(uint16_t));
 memset(&frame,0,sizeof frame);frame.tile_count=2;
 frame.tile_upload[0].source_tile=0;frame.tile_upload[1].source_tile=0;
 assert(wf_vdp1_plan_retain_tile_slots(&frame,resident,resident_bits,3)==WF_BAD_STATE);
 printf("PASS 260 resident remaps and collision/duplicate checks: %zu lookup-clear bytes\n",
        lookup_cleared_bytes);
 return 0;
}
