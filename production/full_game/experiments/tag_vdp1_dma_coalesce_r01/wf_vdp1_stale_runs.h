#ifndef WF_VDP1_STALE_RUNS_H
#define WF_VDP1_STALE_RUNS_H
#include <stdint.h>
/* Baseline contiguous scanner; the failing test defines the grouping change. */
static inline int wf_vdp1_stale_next_run(const uint8_t *bits,unsigned count,unsigned *cursor,unsigned *first,unsigned *last){
 unsigned at=*cursor;
 while(at<count&&!(bits[at>>3]&(1u<<(at&7u))))++at;
 if(at==count){*cursor=count;return 0;}
 *first=at;
 unsigned end=at+1u;
 for(unsigned scan=end;scan<count;){
  if(bits[scan>>3]&(1u<<(scan&7u))){end=++scan;continue;}
  if(scan-end>=2u)break;
  ++scan;
 }
 *last=end;*cursor=end;return 1;
}
#endif
