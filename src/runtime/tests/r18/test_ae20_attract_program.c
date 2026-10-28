#include <assert.h>
#include <string.h>
#include "wf_r18_post_tag.h"

int main(void)
{
    uint8_t ram[0x4000];
    wf_r17_m68k_shadow_t registers;
    wf_r18_prematch_t machine;
    wf_r18_post_tag_t state;
    memset(ram,0,sizeof ram);
    memset(&registers,0,sizeof registers);
    memset(&machine,0,sizeof machine);
    registers.valid=1;
    registers.d[0]=1;
    machine.ram=ram;
    machine.regs=&registers;
    wf_r18_post_tag_context_t context={&machine,0,0};
    wf_r18_post_tag_init(&state,0xae20u);
    assert(wf_r18_post_tag_step(&state,&context)==WF_R18_POST_MORE);
    assert(state.pc==0xae26u);
    assert(wf_r18_post_tag_step(&state,&context)==WF_R18_POST_MORE);
    assert(state.pc==0xae30u);
    assert(wf_r18_post_tag_step(&state,&context)==WF_R18_POST_MORE);
    assert(state.pc==0xae34u && state.saved_d[0]==1u);
    for(unsigned program=1u;program<=2u;program++){
        wf_r18_post_tag_init(&state,0xaf22u);
        ram[0x1278]=0u;ram[0x1279]=(uint8_t)program;
        assert(wf_r18_post_tag_step(&state,&context)==WF_R18_POST_MORE);
        assert(state.pc==0xaf2cu);
    }
    return 0;
}
