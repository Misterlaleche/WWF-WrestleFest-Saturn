#include "wf_music_seq.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
typedef struct {uint64_t hash,count;} digest_t;
extern bool wf_ref_init(wf_music_seq_t *,const uint8_t *,size_t,wf_music_emit_fn,void *,bool);
extern bool wf_ref_send(wf_music_seq_t *,uint8_t);
extern int wf_ref_run(wf_music_seq_t *,uint64_t,unsigned);
static uint8_t rom[65536];static wf_music_seq_t a,b;
static bool capture(unsigned k,unsigned r,unsigned x,uint64_t t,void *u){
 digest_t *d=u;uint64_t value=t^((uint64_t)k<<56)^((uint64_t)r<<48)^((uint64_t)x<<40);
 d->hash^=value;d->hash*=1099511628211ull;++d->count;return true;
}
static void equal_state(void){
 z80 x=a.cpu,y=b.cpu;x.userdata=y.userdata=NULL;x.read_byte=y.read_byte=NULL;x.write_byte=y.write_byte=NULL;x.port_in=y.port_in=NULL;x.port_out=y.port_out=NULL;
 assert(!memcmp(&x,&y,sizeof x));assert(!memcmp(a.ram,b.ram,sizeof a.ram));
 assert(a.cycles==b.cycles&&a.next_tb==b.next_tb&&a.status==b.status&&a.ctl==b.ctl);
}
int main(int argc,char**argv){
 FILE*f=NULL;assert(argc==2&&fopen_s(&f,argv[1],"rb")==0);assert(fread(rom,1,65536,f)==65536);fclose(f);
 assert(wf_ref_init(&a,rom,sizeof rom,NULL,NULL,false));
 assert(wf_ref_run(&a,1000,1)==1&&a.steps==1);
 for(unsigned c=1;c<=18;c++){
  digest_t da={1469598103934665603ull,0},db=da;
  assert(wf_ref_init(&a,rom,sizeof rom,capture,&da,false));
  assert(wf_music_seq_init(&b,rom,sizeof rom,capture,&db,true));
  assert(wf_ref_run(&a,2ull*3579545,20000000)==0);assert(wf_music_seq_run(&b,2ull*3579545,20000000)==0);
  equal_state();assert(da.hash==db.hash&&da.count==db.count);
  assert(wf_ref_send(&a,(uint8_t)c)&&wf_music_seq_send(&b,(uint8_t)c));
  uint64_t origin=a.cycles;
  for(unsigned i=1;i<=48;i++){
   uint64_t target=origin+(uint64_t)i*3579545/4;
   assert(wf_ref_run(&a,target,20000000)==0);assert(wf_music_seq_run(&b,target,20000000)==0);
   equal_state();assert(da.hash==db.hash&&da.count==db.count);
  }
  assert(b.skipped>1000000&&b.steps<a.steps);
  assert(wf_ref_send(&a,0x1f)&&wf_music_seq_send(&b,0x1f));
  assert(wf_ref_run(&a,a.cycles+5ull*3579545,20000000)==0);assert(wf_music_seq_run(&b,b.cycles+5ull*3579545,20000000)==0);
  equal_state();assert(da.hash==db.hash&&da.count==db.count);
 }
 puts("music sequence: all18, exact CPU/RAM/I-O with idle optimization, fade and chunking PASS");return 0;
}
