#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "wp1980_input.h"
#include "wf_r18_frontend_driver.h"

int main(void)
{
    uint8_t ram[0x4000]={0};
    wf_actor_raw_t actor={0};
    wf_r17_m68k_shadow_t regs={.valid=1};
    uint32_t rng=1085u;
    wf_r18_frontend_input_context_t context={.rng_state_1c005c=&rng};
    /* A was held to confirm Royal. A slow frontend samples at the same
     * residue of an eight-display-period pulse, so a timed release aliases. */
    wf_set_be16(&actor,0xa6u,1u);
    memcpy(ram+0x5b0u,&actor,sizeof actor);
    wp1980_frontend_input_begin(100u);
    unsigned fresh=0u;
    for(unsigned poll=0;poll<8u;poll++){
        uint16_t input=wp1980_frontend_input(WF_R18_DRIVER_ROSTER,ram,
            256u+poll*8u,0u,0xffffu);
        assert(wf_r18_frontend_input_64e0(&actor,input,&context,&regs)==0);
        memcpy(ram+0x5b0u,&actor,sizeof actor);
        fresh|=wf_be16(&actor,0xa2u);
    }
    if(!fresh){fputs("FAIL: sampled roster button must release and generate an original $64E0 edge despite display-clock aliasing\n",stderr);return 1;}
    ram[0x5b0u+0x33u]|=1u; /* Original Royal roster owner accepts this edge. */
    assert(wp1980_frontend_input(WF_R18_DRIVER_ROSTER,ram,10000u,0u,0xffffu)&0x10u);
    puts("PASS: original roster input sees release and a fresh A edge across arbitrary display gaps");
    return 0;
}
