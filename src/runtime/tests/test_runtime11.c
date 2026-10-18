#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "wf_saturn_yaul_backend.h"
#include "wf_saturn_combat_present.h"
int main(void){
    wf_saturn_yaul_backend_t b;wf_saturn_yaul_backend_init(&b,0,0);
    const wf_saturn_render_ops_t*o=wf_saturn_yaul_render_ops();assert(o&&o->abort_frame);
    assert(o->begin_frame(&b,1,1,1)==WF_OK);assert(b.active==1);o->abort_frame(&b);assert(b.active==0);assert(b.expected_tiles==0&&b.emitted_tiles==0);
    assert(o->begin_frame(&b,0,0,0)==WF_OK);assert(o->end_frame(&b)==WF_OK);assert(b.active==0);
    assert(wf_saturn_memory_contract_validate()==WF_OK);
    wf_saturn_combat_present_t p;wf_saturn_combat_present_init(&p,0,0);assert(p.backend.active==0);
    puts("PASS test_runtime11");return 0;
}
