/* Detects counting CD boot/repeated fields as game work, losing multivblank
 * gaps, and claiming per-step precision when more than one step was missed. */
#include "wf_audio2_cadence.h"
#include <assert.h>
#include <stdio.h>
int main(void)
{
    wf_audio2_cadence_t p={0};
    wf_audio2_cadence_tick(&p,0,700);
    if(!p.initialized){puts("RED: first logical frame did not establish a combat-only baseline");return 1;}
    wf_audio2_cadence_tick(&p,0,701);
    assert(p.logical_steps==0&&p.elapsed_vblanks==0&&p.updates==0);
    wf_audio2_cadence_tick(&p,1,702);
    wf_audio2_cadence_tick(&p,2,703);
    wf_audio2_cadence_tick(&p,3,706);
    assert(p.first_frame==0&&p.first_vblank==700&&p.logical_steps==3);
    assert(p.elapsed_vblanks==6&&p.updates==3&&p.max_gap==3);
    assert(p.gaps[1]==1&&p.gaps[2]==1&&p.gaps[3]==1&&!p.unobserved_steps);
    wf_audio2_cadence_tick(&p,5,708);
    assert(p.unobserved_steps==1&&p.logical_steps==5&&p.elapsed_vblanks==8);
    wf_audio2_cadence_tick(&p,2,709);
    assert(p.regressions==1&&p.logical_steps==5&&p.elapsed_vblanks==8);
    puts("PASS combat baseline, repeated fields, exact observed gaps and missed-step disclosure");
    return 0;
}
