#include <assert.h>
#include <string.h>
#include "wf_r17_contract.h"
int main(void){wf_runtime_t r;wf_actor_raw_t a;uint8_t plain[]={0,1,0,0,0,0,0,0};uint8_t packed[]={0x40,1,0,0};memset(&r,0,sizeof r);memset(&a,0,sizeof a);r.m68k_regs.valid=1;r.m68k_regs.d[6]=0xdeadbeef;r.m68k_regs.d[7]=0xcafebabe;
assert(wf_r17_render_record_shadow(&r,&a,plain,sizeof plain)==WF_OK);assert(r.m68k_regs.d[6]==0&&r.m68k_regs.d[7]==0);
wf_set_be16(&a,WF_A_SCREEN_X,0x200);assert(wf_r17_render_record_shadow(&r,&a,plain,sizeof plain)==WF_OK);assert(r.m68k_regs.d[6]==5);
wf_set_be16(&a,WF_A_SCREEN_X,0);wf_set_be16(&a,WF_A_SCREEN_Y,0x200);assert(wf_r17_render_record_shadow(&r,&a,plain,sizeof plain)==WF_OK);assert(r.m68k_regs.d[6]==4);
assert(wf_r17_render_record_shadow(&r,&a,packed,sizeof packed)==WF_OK);assert(r.m68k_regs.d[6]==0x1c1610);
packed[0]=0x42;wf_r17_m68k_shadow_t before=r.m68k_regs;assert(wf_r17_render_record_shadow(&r,&a,packed,sizeof packed)==WF_BLOCKED);assert(memcmp(&before,&r.m68k_regs,sizeof before)==0);return 0;}
