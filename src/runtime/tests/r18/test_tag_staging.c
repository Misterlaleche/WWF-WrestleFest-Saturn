/* Compile this TU instead of tag_6e34.c to check stable in-place grouping. */
#include "../../src/wf_r17_tag_6e34.c"
#include <assert.h>
#include <stdio.h>
static wf_participant_machine_t stage_machine;
static wf_r17_fg0_write_t original_writes[WF_PARTICIPANT_WRITES];
static void check(size_t count,unsigned split,unsigned pattern){
 size_t nf,nv,expect_fg=0,fi=0,vi;
 memset(stage_machine.writes,0x5a,sizeof stage_machine.writes);stage_machine.write_count=count;
 for(size_t i=0;i<count;i++){
  int foreground=pattern==0?i<split:pattern==1?(i&1u)==0u:pattern==2?(i%3u)==1u:pattern==3?i+1u==count:i==0u;
  stage_machine.writes[i]=(wf_r17_fg0_write_t){(foreground?0xc0000u:0x180000u)+(uint32_t)i*4u,(uint32_t)i^0xa55aa55au,(uint8_t)((i&1u)?2u:4u)};
  expect_fg+=(unsigned)foreground;
 }
 memcpy(original_writes,stage_machine.writes,sizeof original_writes);
 tag_stage_outputs(&stage_machine,&nf,&nv);assert(nf==expect_fg&&nf+nv==count);
 vi=nf;
 for(size_t i=0;i<count;i++){
  size_t out=original_writes[i].addr<0x180000u?fi++:vi++;
  assert(!memcmp(&stage_machine.writes[out],&original_writes[i],sizeof stage_machine.writes[out]));
 }
 assert(fi==nf&&vi==count);
 const uint8_t*tail=(const uint8_t*)(stage_machine.writes+count);
 for(size_t i=0;i<sizeof stage_machine.writes-count*sizeof stage_machine.writes[0];i++)assert(tail[i]==0x5au);
}
int main(void){
 unsigned cases=0;assert(sizeof stage_machine.writes==WF_PARTICIPANT_WRITES*sizeof stage_machine.writes[0]);
 /* Every possible partition at full capacity, including either empty side. */
 for(unsigned split=0;split<=WF_PARTICIPANT_WRITES;split++){check(WF_PARTICIPANT_WRITES,split,0);cases++;}
 for(unsigned pattern=1;pattern<=4;pattern++)for(unsigned count=0;count<=WF_PARTICIPANT_WRITES;count++){check(count,0,pattern);cases++;}
 printf("PASS TAG staging:%u partitions/capacities; bytes=%zu; stable group order, no tail writes\n",cases,sizeof stage_machine.writes);return 0;
}
