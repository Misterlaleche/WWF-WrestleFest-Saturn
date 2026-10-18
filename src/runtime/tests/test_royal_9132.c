#include "wf_r17_royal_9132.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
typedef struct capture {uint16_t input[4];unsigned reads,batches,fail_read,fail_batch;size_t writes;} capture;
static wf_status_t read16(wf_runtime_t*r,uint32_t a,uint16_t*v,void*u){capture*c=u;assert(a>=0x140020&&a<=0x140026);c->reads++;r->m68k_regs.d[7]=0; if(c->fail_read==c->reads)return WF_BLOCKED;*v=c->input[(a-0x140020)/2];return WF_OK;}
static wf_status_t batch(wf_runtime_t*r,const wf_r17_fg0_write_t*w,size_t n,void*u){capture*c=u;c->batches++;c->writes=n;assert(n>0);assert(w[0].addr==0xc1731&&w[0].value==7);r->m68k_regs.d[0]=0;return c->fail_batch?WF_BLOCKED:WF_OK;}
static wf_status_t commit(wf_runtime_t*r,const wf_r17_fg0_write_t*w,size_t n,const wf_r17_fg0_write_t*v,size_t vn,void*u){assert(v==NULL&&vn==0);return batch(r,w,n,u);}
static void setup(wf_runtime_t*r,wf_r17_participant_io_t*io,capture*c){memset(r,0,sizeof(*r));memset(io,0,sizeof(*io));memset(c,0,sizeof(*c));for(unsigned i=0;i<4;i++)c->input[i]=0xffff;for(unsigned i=0;i<8;i++)r->m68k_regs.d[i]=0x12345678+i;r->m68k_regs.valid=1;io->input_word=read16;io->fg0_batch=batch;io->user=c;}
static uint16_t raw(wf_runtime_t*r,unsigned a){return (uint16_t)((r->g.royal_selector_1576[a-0x1576]<<8)|r->g.royal_selector_1576[a-0x1575]);}
int main(void){wf_runtime_t rt,snap;wf_r17_participant_io_t io;capture c;
 setup(&rt,&io,&c);snap=rt;assert(wf_r17_owner_9132(&rt,NULL,0,&io)==WF_OK);assert(memcmp(&rt.m68k_regs,&snap.m68k_regs,sizeof(rt.m68k_regs))==0);assert(rt.g.global_167b==2);assert(raw(&rt,0x15b4)==10);assert(c.reads==4&&c.batches==1);
 setup(&rt,&io,&c);rt.g.royal_slots_1586[0]=0x8000;assert(wf_r17_owner_9132(&rt,NULL,0,&io)==WF_OK);assert(rt.g.royal_slots_1586[9]==0x8000);assert(raw(&rt,0x15d0)==319);assert(raw(&rt,0x15b6)==5);
 setup(&rt,&io,&c);rt.g.royal_slots_1586[0]=0x8000;c.input[0]=0xffef;assert(wf_r17_owner_9132(&rt,NULL,0,&io)==WF_OK);assert(rt.g.royal_slots_1586[0]==0x4000);assert(wf_be16(&rt.actors[0],0x32)==0x8003);assert(wf_be16(&rt.actors[0],0x66)==110);assert(wf_be16(&rt.actors[0],0x6e)==110);assert(wf_be32(&rt.actors[0],0x8a)==0x140020);assert(rt.g.royal_occupancy[0]==1);
 setup(&rt,&io,&c);snap=rt;c.fail_read=2;assert(wf_r17_owner_9132(&rt,NULL,0,&io)==WF_BLOCKED);assert(memcmp(&rt,&snap,sizeof rt)==0);assert(c.batches==0);
 setup(&rt,&io,&c);snap=rt;c.fail_batch=1;assert(wf_r17_owner_9132(&rt,NULL,0,&io)==WF_BLOCKED);assert(memcmp(&rt,&snap,sizeof rt)==0);
 setup(&rt,&io,&c);io.fg0_batch=NULL;io.commit_batch=commit;assert(wf_r17_owner_9132(&rt,NULL,0,&io)==WF_OK);assert(c.batches==1);
 puts("PASS Royal native selection, confirmation, aliasing, register preservation and transaction rollback");return 0;}
