#include "wf_music_seq.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <inttypes.h>
typedef struct{uint64_t hash,count;} digest_t;
extern bool wf_ref_init(wf_music_seq_t*,const uint8_t*,size_t,wf_music_emit_fn,void*,bool);
extern bool wf_ref_send(wf_music_seq_t*,uint8_t);
extern int wf_ref_run(wf_music_seq_t*,uint64_t,unsigned);
static wf_music_seq_t a,b;static uint8_t rom[65536];static unsigned cmd,second;
static bool event(unsigned k,unsigned r,unsigned x,uint64_t t,void*u){digest_t*d=u;d->hash^=t^((uint64_t)k<<56)^((uint64_t)r<<48)^((uint64_t)x<<40);d->hash*=1099511628211ull;++d->count;return true;}
static void normalize(z80*z){z->userdata=NULL;z->read_byte=NULL;z->write_byte=NULL;z->port_in=NULL;z->port_out=NULL;}
static void equal(digest_t*da,digest_t*db){
 z80 x=a.cpu,y=b.cpu;normalize(&x);normalize(&y);
 bool ok=!memcmp(&x,&y,sizeof x)&&!memcmp(a.ram,b.ram,sizeof a.ram)&&a.cycles==b.cycles&&a.last_cpu==b.last_cpu&&a.next_tb==b.next_tb&&a.ym_addr==b.ym_addr&&a.status==b.status&&a.tb==b.tb&&a.ctl==b.ctl&&a.latch==b.latch&&a.bank==b.bank&&a.command_pending==b.command_pending&&a.error==b.error&&da->hash==db->hash&&da->count==db->count;
 if(!ok){fprintf(stderr,"FAIL cmd%u checkpoint%us cyclesA=%" PRIu64 " B=%" PRIu64 " eventA=%" PRIu64 " B=%" PRIu64 "\n",cmd,second,a.cycles,b.cycles,da->count,db->count);fflush(stderr);}
 assert(ok);
}
int main(int argc,char**argv){
 FILE*f=NULL,*log=NULL;assert(argc==3&&fopen_s(&f,argv[1],"rb")==0);assert(fread(rom,1,sizeof rom,f)==sizeof rom);fclose(f);
 assert(!fopen_s(&log,argv[2],"w"));fputs("cmd,checkpoint_s,phase,cycles,event_count,event_hash,candidate_calls,candidate_exhaustions\n",log);
 uint64_t checks=0,calls=0,exhaustions=0;
 for(cmd=1;cmd<=18;cmd++){
  digest_t da={1469598103934665603ull,0},db=da;
  assert(wf_ref_init(&a,rom,sizeof rom,event,&da,false));assert(wf_music_seq_init(&b,rom,sizeof rom,event,&db,true));
  while(wf_ref_run(&a,2ull*3579545,20000000)>0){}while(wf_music_seq_run(&b,2ull*3579545,4096)>0){}
  second=0;equal(&da,&db);++checks;
  assert(wf_ref_send(&a,(uint8_t)cmd)&&wf_music_seq_send(&b,(uint8_t)cmd));
  uint64_t origin=b.cycles,target_cycle=origin;uint32_t fraction=0;unsigned fs=0,next_grid=512;
  for(second=1;second<=608;second++){
   if(second==601)assert(wf_ref_send(&a,0x1f)&&wf_music_seq_send(&b,0x1f));
   unsigned checkpoint=second*44100u;
   while(fs<checkpoint){
    unsigned next=next_grid<checkpoint?next_grid:checkpoint;
    uint64_t numerator=(uint64_t)(next-fs)*3579545u+fraction;
    target_cycle+=numerator/44100u;fraction=(uint32_t)(numerator%44100u);fs=next;
    assert(target_cycle==origin+(uint64_t)fs*3579545u/44100u);
    int result;do{result=wf_music_seq_run(&b,target_cycle,4096);assert(result>=0);++calls;if(result>0)++exhaustions;}while(result>0);
    if(next==next_grid)next_grid+=512;
   }
   int result;do{result=wf_ref_run(&a,origin+(uint64_t)second*3579545u,20000000);assert(result>=0);}while(result>0);
   equal(&da,&db);++checks;
   fprintf(log,"%u,%u,%s,%" PRIu64 ",%" PRIu64 ",%016" PRIx64 ",%" PRIu64 ",%" PRIu64 "\n",cmd,second,second<=600?"score":"fade",b.cycles,db.count,db.hash,calls,exhaustions);
  }
  printf("cmd%02x PASS600s+8sfade checkpoints608 events=%" PRIu64 " hash=%016" PRIx64 "\n",cmd,db.count,db.hash);fflush(stdout);
 }
 fclose(log);printf("PASS Q512/B4096:18x600s+8sfade, %" PRIu64 " exact frozen-R01 CPU/RAM/timer/event checkpoints, candidate calls=%" PRIu64 " exhaustions=%" PRIu64 "; globalFs512 grid preserved with common1s observation splits\n",checks,calls,exhaustions);return 0;
}
