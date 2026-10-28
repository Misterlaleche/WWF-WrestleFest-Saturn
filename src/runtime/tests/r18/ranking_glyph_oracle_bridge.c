#include "wf_r18_ranking_glyph.h"
#include <string.h>
typedef struct bus {uint8_t*memory;uint32_t*events;unsigned calls;int fail;} bus_t;
static wf_status_t store(void*u,uint32_t at,uint32_t v,unsigned n){
 bus_t*b=u;unsigned i=b->calls++;
 if(i<16u){b->events[i*3]=at;b->events[i*3+1]=v;b->events[i*3+2]=n;}
 if((int)i==b->fail||at<0xc0000u||at>0xc4000u-n)return WF_BLOCKED;
 for(unsigned j=0;j<n;j++)b->memory[at+j]=(uint8_t)(v>>(8u*(n-1u-j)));
 return WF_OK;
}
int ranking_glyph_oracle(uint8_t*mem,uint32_t*d,uint32_t*a,unsigned owner,int fail,uint32_t*diag,uint32_t*events){
 wf_r17_m68k_shadow_t regs={.valid=1};memcpy(regs.d,d,32);
 wf_r18_prematch_t m={.ram=mem+0x1c0000,.regs=&regs};memcpy(m.a,a,28);
 bus_t bus={mem,events,0,fail};wf_r18_ranking_glyph_context_t c={&m,store,&bus,0,0};
 int status=wf_r18_ranking_glyph_call(&c,owner);diag[0]=c.blocked_dependency;diag[1]=c.blocked_address;diag[2]=bus.calls;diag[3]=m.dependency;
 memcpy(d,regs.d,32);memcpy(a,m.a,28);return status;
}
int ranking_glyph_bad_context(void){
 uint8_t raw=0;wf_r17_m68k_shadow_t regs={.valid=1};wf_r18_prematch_t m={.ram=&raw,.regs=&regs};
 wf_r18_ranking_glyph_context_t c={.machine=&m,.store_bus=store};
 if(wf_r18_ranking_glyph_call(NULL,0x6cfa)!=WF_BAD_STATE)return 1;
 c.machine=NULL;if(wf_r18_ranking_glyph_call(&c,0x6cfa)!=WF_BAD_STATE)return 2;
 c.machine=&m;m.ram=NULL;if(wf_r18_ranking_glyph_call(&c,0x6cfa)!=WF_BAD_STATE)return 3;
 m.ram=&raw;m.regs=NULL;if(wf_r18_ranking_glyph_call(&c,0x6cfa)!=WF_BAD_STATE)return 4;
 m.regs=&regs;regs.valid=0;if(wf_r18_ranking_glyph_call(&c,0x6cfa)!=WF_BAD_STATE)return 5;
 return 0;
}
int ranking_glyph_missing_dependencies(void){
 uint8_t raw=0x5a;wf_r17_m68k_shadow_t regs={.valid=1};regs.d[3]=0xabcdef01u;
 wf_r18_prematch_t m={.ram=&raw,.regs=&regs};m.a[2]=0xff0c0120u;
 wf_r18_ranking_glyph_context_t c={.machine=&m};
 if(wf_r18_ranking_glyph_call(&c,0x6d76)!=WF_BLOCKED||c.blocked_dependency!=0x6d76u||c.blocked_address!=0xc0121u||m.dependency!=0x6d76u)return 1;
 if(regs.d[3]!=0xabcdef01u||m.a[2]!=0xff0c0120u||raw!=0x5a)return 2;
 if(wf_r18_ranking_glyph_call(&c,0xdeadbeefu)!=WF_BLOCKED||c.blocked_dependency!=0xdeadbeefu||c.blocked_address||m.dependency!=0xdeadbeefu)return 3;
 return regs.d[3]==0xabcdef01u&&m.a[2]==0xff0c0120u&&raw==0x5a?0:4;
}
