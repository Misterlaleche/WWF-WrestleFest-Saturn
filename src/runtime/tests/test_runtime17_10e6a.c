#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "wf_gameplay_bindings.h"
#include "wf_r17_transient_reset_10e6a.h"

static void test_exact_clear(void){
 wf_runtime_t rt;unsigned i,j;memset(&rt,0,sizeof(rt));
 for(i=0;i<WF_R17_TRANSIENT_RECORDS;i++){
  for(j=0;j<WF_R17_TRANSIENT_STRIDE;j++)rt.transient_1258[i][j]=(uint8_t)(0x80u|((i+j)&0x7fu));
 }
 for(i=0;i<8;i++)rt.m68k_regs.d[i]=0x12345678u+i;
 uint8_t before[WF_R17_TRANSIENT_RECORDS][WF_R17_TRANSIENT_STRIDE];memcpy(before,rt.transient_1258,sizeof(before));
 assert(wf_r17_transient_reset_10e6a(&rt)==WF_OK);
 assert(rt.m68k_regs.d[0]==0x1234ffffu);
 for(i=1;i<8;i++)assert(rt.m68k_regs.d[i]==0x12345678u+i);
 for(i=0;i<WF_R17_TRANSIENT_RECORDS;i++){
  assert(rt.transient_1258[i][0]==(uint8_t)(before[i][0]&0x7fu));
  for(j=1;j<WF_R17_TRANSIENT_STRIDE;j++)assert(rt.transient_1258[i][j]==before[i][j]);
 }
 puts("PASS $10E6A BCLR #7,+$00 across 12 x $2A records only");
}
static void test_preserves_low7(void){
 wf_runtime_t rt;memset(&rt,0,sizeof(rt));
 for(unsigned i=0;i<WF_R17_TRANSIENT_RECORDS;i++)rt.transient_1258[i][0]=(uint8_t)(0x80u|i);
 assert(wf_r17_transient_reset_10e6a(&rt)==WF_OK);
 for(unsigned i=0;i<WF_R17_TRANSIENT_RECORDS;i++)assert(rt.transient_1258[i][0]==(uint8_t)i);
 puts("PASS $10E6A preserves bits0..6");
}
static void test_scheduler(void){
 wf_runtime_t rt;wf_gameplay_binding_context_t c;wf_phase_binding_t b[40];
 size_t n=wf_gameplay_bindings_build(&c,b,40u);assert(n>=29u);wf_runtime_init(&rt,b,n);rt.g.rng_state_valid=1u;rt.g.rng_state=0x13579bdfu;
 c.cabinet_words_valid=1u;c.cabinet_word_140020=c.cabinet_word_140022=0xffffu;c.camera_valid=1u;
 c.camera.scroll0_x=0x0140;c.camera.scroll1_x=0x0140;c.camera.scroll0_y=0x0200;c.camera.scroll1_y=0x0200;
 rt.g.mode_0161=1u;wf_r17_referee_init_10718(&rt);wf_r17_fixed_four_init_1004c(&rt);
 for(unsigned i=0;i<WF_R17_TRANSIENT_RECORDS;i++)rt.transient_1258[i][0]=0xffu;
 assert(wf_runtime_frame(&rt)==WF_BLOCKED);assert(rt.trace_count==23u);assert(rt.trace[22]==0x008cf8u);
 assert(rt.trace[14]==0x010120u&&rt.trace[15]==0x010e6au&&rt.trace[16]==0x007a00u);
 for(unsigned i=0;i<WF_R17_TRANSIENT_RECORDS;i++)assert(rt.transient_1258[i][0]==0x7fu);
 puts("PASS scheduler crosses $10E6A and crosses $7A00 and blocks at $8CF8");
}
int main(void){test_exact_clear();test_preserves_low7();test_scheduler();puts("PASS RUNTIME-17 $10E6A transient reset owner");return 0;}
