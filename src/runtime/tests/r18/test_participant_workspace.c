#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "wf_r17_tag_6e34.h"
#include "wf_r17_royal_9132.h"
enum {TAG=0,ROYAL=1,INPUT=0,COMMIT=1};
typedef struct capture {
 unsigned owner,inner,boundary,same,probe,attempted,fail,reads,commits,committed;
 uint16_t hw[4];uint32_t hash;
} capture;
static wf_runtime_t separate,initial,expected,actual;
static wf_r17_fg0_write_t fg_copy[WF_PARTICIPANT_WRITES],vid_copy[WF_PARTICIPANT_WRITES];
static wf_status_t read16(wf_runtime_t*,uint32_t,uint16_t*,void*);
static wf_status_t commit(wf_runtime_t*,const wf_r17_fg0_write_t*,size_t,const wf_r17_fg0_write_t*,size_t,void*);
static void setup(wf_runtime_t*r,wf_r17_participant_io_t*io,capture*c,unsigned owner){
 memset(r,0,sizeof *r);memset(io,0,sizeof *io);memset(c,0,sizeof *c);c->owner=owner;c->hash=2166136261u;
 for(unsigned i=0;i<4;i++)c->hw[i]=0xffffu;
 for(unsigned i=0;i<8;i++)r->m68k_regs.d[i]=0x12345670u+i;r->m68k_regs.valid=1;
 r->g.credited_route=1;r->g.config_0066=owner==TAG?0x804u:4u;r->g.credit_count_004f=1;r->g.mode_0161=(uint8_t)owner;
 if(owner==TAG){r->actors[2].b[0]=0x80u;c->hw[0]=0xff7fu;}
 io->input_word=read16;io->commit_batch=commit;io->user=c;io->blocked_dependency_rom=0xabcdef01u;
}
static wf_status_t call(unsigned owner,wf_runtime_t*r,wf_r17_participant_io_t*io){return owner==TAG?wf_r17_owner_6e34(r,io):wf_r17_owner_9132(r,NULL,0,io);}
static void attempt(wf_runtime_t*r,capture*c){
 if(!c->probe||c->attempted)return;c->attempted=1;
 wf_r17_participant_io_t io;capture nested;setup(&separate,&io,&nested,c->inner);
 wf_runtime_t*target=c->same?r:&separate;wf_runtime_t before=*r,other_before=separate;
 wf_r17_participant_io_t io_before=io;
 wf_status_t status=call(c->inner,target,&io);
 assert(status==WF_BAD_STATE);
 assert(!memcmp(r,&before,sizeof *r));assert(!memcmp(&separate,&other_before,sizeof separate));
 assert(!memcmp(&io,&io_before,sizeof io));assert(nested.reads==0&&nested.commits==0);
}
static wf_status_t read16(wf_runtime_t*r,uint32_t at,uint16_t*v,void*u){
 capture*c=u;c->reads++;if(c->boundary==INPUT)attempt(r,c);
 if(c->fail==1u&&c->reads==1u)return WF_BLOCKED;
 assert(at>=0x140020u&&at<=0x140026u&&!(at&1u));*v=c->hw[(at-0x140020u)/2u];return WF_OK;
}
static void digest(capture*c,const wf_r17_fg0_write_t*w,size_t n){
 for(size_t i=0;i<n;i++){
  c->hash=(c->hash^w[i].addr)*16777619u;c->hash=(c->hash^w[i].value)*16777619u;c->hash=(c->hash^w[i].width)*16777619u;
 }
}
static wf_status_t commit(wf_runtime_t*r,const wf_r17_fg0_write_t*f,size_t nf,const wf_r17_fg0_write_t*v,size_t nv,void*u){
 capture*c=u;c->commits++;assert(nf<=WF_PARTICIPANT_WRITES&&nv<=WF_PARTICIPANT_WRITES);
 if(c->owner==TAG)assert((uintptr_t)v==(uintptr_t)(f+nf));
 if(nf)memcpy(fg_copy,f,nf*sizeof *f);if(nv)memcpy(vid_copy,v,nv*sizeof *v);
 if(c->boundary==COMMIT)attempt(r,c);
 if(nf)assert(!memcmp(fg_copy,f,nf*sizeof *f));if(nv)assert(!memcmp(vid_copy,v,nv*sizeof *v));
 digest(c,f,nf);digest(c,v,nv);if(c->fail==2u)return WF_BLOCKED;c->committed++;return WF_OK;
}
int main(void){
 unsigned cases=0,reuse=0;
 for(unsigned outer=0;outer<2;outer++)for(unsigned inner=0;inner<2;inner++)for(unsigned boundary=0;boundary<2;boundary++)for(unsigned same=0;same<2;same++)for(unsigned fail=0;fail<3;fail++){
  if(boundary==COMMIT&&fail==1u)continue;
  wf_r17_participant_io_t base_io,probe_io;capture base,probe;
  setup(&expected,&base_io,&base,outer);initial=expected;base.boundary=boundary;base.fail=fail;
  wf_status_t want=call(outer,&expected,&base_io);assert(want==(fail?WF_BLOCKED:WF_OK));
  if(fail)assert(!memcmp(&expected,&initial,sizeof expected));
  setup(&actual,&probe_io,&probe,outer);probe.boundary=boundary;probe.fail=fail;probe.inner=inner;probe.same=same;probe.probe=1;
  assert(call(outer,&actual,&probe_io)==want);assert(probe.attempted==1u);
  assert(!memcmp(&actual,&expected,sizeof actual));assert(probe_io.blocked_dependency_rom==base_io.blocked_dependency_rom);
  assert(probe.reads==base.reads&&probe.commits==base.commits&&probe.committed==base.committed&&probe.hash==base.hash);
  if(fail)assert(probe.committed==0u);
  /* Both owners can acquire again after success or any outer failure. */
  for(unsigned next=0;next<2;next++){wf_r17_participant_io_t io;capture c;setup(&actual,&io,&c,next);assert(call(next,&actual,&io)==WF_OK);reuse++;}
  cases++;
 }
 /* Royal's early missing-output exit must release before either next owner. */
 for(unsigned next=0;next<2;next++){
  wf_r17_participant_io_t io;capture c;setup(&actual,&io,&c,ROYAL);initial=actual;io.commit_batch=NULL;
  assert(call(ROYAL,&actual,&io)==WF_BLOCKED&&io.blocked_dependency_rom==0x9658u);
  assert(!memcmp(&actual,&initial,sizeof actual)&&c.reads==0u&&c.commits==0u);
  setup(&actual,&io,&c,next);assert(call(next,&actual,&io)==WF_OK);
 }
#ifdef WF_PARTICIPANT_LEASE_API
 wf_participant_workspace_t*w=NULL,*same=NULL,*unchanged=(wf_participant_workspace_t*)(uintptr_t)1u;
 assert(wf_participant_workspace_acquire(WF_PARTICIPANT_TAG,&w)==WF_OK&&w);
 memset(w,0xa5,sizeof *w);static wf_participant_workspace_t snapshot;snapshot=*w;
 assert(wf_participant_workspace_acquire(WF_PARTICIPANT_ROYAL,&unchanged)==WF_BAD_STATE&&unchanged==(wf_participant_workspace_t*)(uintptr_t)1u);
 assert(!memcmp(w,&snapshot,sizeof *w));assert(wf_participant_workspace_release(WF_PARTICIPANT_ROYAL,w)==WF_BAD_STATE);
 assert(wf_participant_workspace_acquire(WF_PARTICIPANT_TAG,&unchanged)==WF_BAD_STATE);
 assert(wf_participant_workspace_release(WF_PARTICIPANT_TAG,w)==WF_OK);
 assert(wf_participant_workspace_acquire(WF_PARTICIPANT_ROYAL,&same)==WF_OK&&same==w);
 assert(wf_participant_workspace_release(WF_PARTICIPANT_ROYAL,same)==WF_OK);
 assert(wf_participant_workspace_acquire(0u,&same)==WF_BAD_STATE);
 assert(wf_participant_workspace_acquire(WF_PARTICIPANT_TAG,NULL)==WF_BAD_STATE);
 assert(wf_participant_workspace_release(WF_PARTICIPANT_TAG,NULL)==WF_BAD_STATE);
 printf("lease bytes HOST: machine=%zu runtime=%zu workspace=%zu\n",sizeof(wf_participant_machine_t),sizeof(wf_runtime_t),sizeof(wf_participant_workspace_t));
#endif
 printf("PASS participant workspace:%u nested owner transactions,%u reacquisitions; input/commit same/cross runtime, output preservation and outer rollback\n",cases,reuse);return 0;
}
