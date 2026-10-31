#include "wf_r18_video.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
extern wf_status_t wf_video_batch_reference(wf_runtime_t*,const wf_r17_fg0_write_t*,size_t,void*);
static wf_r18_video_t fast,reference;
static unsigned calls;
static int ring(unsigned layer,unsigned cell,uint8_t*bytes,wf_r18_background_op_t op){
 (void)layer;(void)cell;(void)bytes;(void)op;calls++;return 0;
}
static void check(const wf_r17_fg0_write_t*w,size_t n){
 reference=fast;unsigned before=calls;wf_status_t a=wf_video_batch_reference(NULL,w,n,&reference);
 unsigned old=calls-before;before=calls;wf_status_t b=wf_r18_video_batch(NULL,w,n,&fast);
 assert(a==b&&old==calls-before&&!memcmp(&fast,&reference,sizeof fast));
}
int main(void){
 memset(&fast,0,sizeof fast);fast.failed_address=0xdeadbeef;
 const unsigned bases[]={0x180000,0x182000,0x186000,0x188000};
 for(unsigned repeat=0;repeat<2;repeat++)for(unsigned role=0;role<4;role++)
  for(unsigned bank=0;bank<16;bank++)for(unsigned pen=0;pen<16;pen++){
   wf_r17_fg0_write_t w={bases[role]+bank*128+pen*2,0x12340000u|(bank<<8)|(pen<<4)|role,2};check(&w,1);check(&w,1);
  }
 for(unsigned role=0;role<4;role++)for(int offset=-8;offset<0x808;offset++)for(unsigned width=0;width<=5;width++){
  wf_r17_fg0_write_t w={bases[role]+offset,0xcafeba98,width};check(&w,1);
 }
 fast.ring_cell=ring;
 wf_r17_fg0_write_t batch[]={{0x80000,0x12345678,4},{0xc0001,0xab,1},{0x180000,0xabc,2}};
 check(batch,3);check(batch,1);batch[2].addr=0xffffffff;check(batch,3);
 check(NULL,0);assert(wf_r18_video_batch(NULL,NULL,1,&fast)==WF_BAD_STATE);
 assert(wf_r18_video_batch(NULL,batch,1,NULL)==WF_BAD_STATE);
 puts("video bus: full state/dirty/errors/ring differential PASS");return 0;
}
