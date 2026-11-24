/* Original-ROM compact-object fixture -> real native return initializer and
 * fixed-four phase. This catches a stale named +$1C alias restoring a latch
 * after $1004A cleared the compact record, preserving the exterior pose. */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "wf_gameplay_bindings.h"
#include "wf_r17_fixed_four_10120.h"

static wf_runtime_t runtime;
static wf_gameplay_binding_context_t gameplay;

int main(int argc,char **argv)
{
    assert(argc==3);
    wf_actor_raw_t *objects[4]={&runtime.fixed_four_1134,&runtime.referee_aux,
        &runtime.fixed_four_1194,&runtime.fixed_four_11c4};
    uint16_t *aliases[4]={&runtime.g.global_1150,&runtime.g.global_1180,
        &runtime.g.global_11b0,&runtime.g.global_11e0};
    FILE *input=fopen(argv[1],"rb");assert(input);
    for(unsigned i=0;i<4;i++){
        assert(fread(objects[i]->b,1,0x30u,input)==0x30u);
        *aliases[i]=wf_be16(objects[i],WF_A_LATCHED);
    }
    assert(fgetc(input)==EOF);assert(!fclose(input));
    runtime.m68k_regs.valid=1;
    gameplay.camera_valid=1;
    gameplay.camera.scroll0_x=0x1e0;
    gameplay.camera.scroll0_y=0x230;
    /* The real $F98C return barrier calls this producer. The next scheduled
     * fixed-four phase is the first consumer of its compact/named state. */
    wf_r17_fixed_four_init_1004c(&runtime);
    FILE *output=fopen(argv[2],"wb");assert(output);
    for(unsigned i=0;i<4;i++){
        uint8_t word[2]={(uint8_t)(*aliases[i]>>8),(uint8_t)*aliases[i]};
        assert(fwrite(word,1,2,output)==2);
    }
    assert(wf_phase_fixed_four_10120(&runtime,0x10120u,&gameplay)==WF_OK);
    for(unsigned i=0;i<4;i++)assert(fwrite(objects[i]->b,1,0x30u,output)==0x30u);
    assert(!fclose(output));
    return 0;
}
