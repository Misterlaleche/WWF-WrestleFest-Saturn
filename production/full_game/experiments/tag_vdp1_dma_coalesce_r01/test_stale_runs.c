#include "wf_vdp1_stale_runs.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static void set(uint8_t*b,unsigned x){b[x>>3]|=(uint8_t)(1u<<(x&7u));}
static unsigned runs(const uint8_t*b,unsigned count,unsigned out[][2]){
 unsigned cursor=0,first,last,n=0;
 while(wf_vdp1_stale_next_run(b,count,&cursor,&first,&last)){
  assert(first<last&&last<=count&&n<256);out[n][0]=first;out[n][1]=last;++n;
 }
 return n;
}
int main(void){
 uint8_t bits[32]={0};unsigned out[256][2];
 assert(runs(bits,256,out)==0);
 set(bits,0);set(bits,3);set(bits,6);set(bits,10);set(bits,255);
 unsigned n=runs(bits,256,out);
 assert(n==3);assert(out[0][0]==0&&out[0][1]==7);
 assert(out[1][0]==10&&out[1][1]==11);
 assert(out[2][0]==255&&out[2][1]==256);
 uint8_t stage[256*128],old[256*128],newer[256*128];
 for(unsigned i=0;i<sizeof stage;i++)stage[i]=(uint8_t)(i*17u+3u);
 memcpy(old,stage,sizeof old);memcpy(newer,stage,sizeof newer);
 for(unsigned slot=0;slot<256;slot++)if(bits[slot>>3]&(1u<<(slot&7u))){
  memset(old+slot*128,0x5a,128);memset(newer+slot*128,0x5a,128);
 }
 for(unsigned slot=0;slot<256;slot++)if(bits[slot>>3]&(1u<<(slot&7u)))
  memcpy(old+slot*128,stage+slot*128,128);
 for(unsigned i=0;i<n;i++)memcpy(newer+out[i][0]*128,stage+out[i][0]*128,(out[i][1]-out[i][0])*128);
 assert(!memcmp(old,newer,sizeof old));
 puts("PASS stale runs and rendered bytes");return 0;
}
