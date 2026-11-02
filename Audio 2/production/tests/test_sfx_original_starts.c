/* Compare actual native start identities/counts with the full original Z80
 * board reference. The private fixture is generated from the user's ROM run.
 * It is intentionally independent of the preserved A15 command model.
 */
#include "wf_scsp_sfx.h"
#include "sfx_original_fixture.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
static unsigned command_index,starts,mismatches;
static wf_scsp_sfx_t engine;
static bool write16(uint32_t address,uint16_t value,void *user)
{(void)address;(void)value;(void)user;return true;}
static bool control(unsigned voice,bool on,unsigned volume,void *user)
{
    (void)user;if(!on)return true;
    const original_start_case_t *c=&original_start_cases[command_index];
    if(starts>=c->count){mismatches++;starts++;return true;}
    const original_start_t *expected=&original_starts[c->offset+starts++];
    if(engine.pcm[voice].bank!=expected->bank||engine.pcm[voice].sample_id!=expected->sample||volume!=expected->volume)mismatches++;
    return true;
}
int main(int argc,char **argv)
{
    if(argc!=2)return 2;
    FILE *f=fopen(argv[1],"rb");assert(f);fseek(f,0,SEEK_END);size_t n=(size_t)ftell(f);rewind(f);
    uint8_t *pack=malloc(n);assert(pack&&fread(pack,1,n,f)==n);fclose(f);
    unsigned failed=0;
    for(command_index=0;command_index<98;command_index++){
        const original_start_case_t *c=&original_start_cases[command_index];starts=mismatches=0;
        wf_scsp_sfx_init(&engine,write16,NULL);engine.pcm_control=control;
        assert(wf_scsp_sfx_bind(&engine,pack,n));
        assert(wf_scsp_sfx_command(&engine,(uint8_t)(command_index+0x20))>=0);
        for(unsigned frame=0;frame<c->seconds*44100u;frame+=16){wf_scsp_sfx_work(&engine);wf_scsp_sfx_advance(&engine,16);}
        assert(!engine.fault&&!engine.underruns);
        if(starts!=c->count||mismatches){
            fprintf(stderr,"FAIL command%02X actualstarts%u original%u mismatches%u\n",command_index+0x20,starts,c->count,mismatches);failed++;
        }
    }
    free(pack);
    printf("%s:98 original full-board start identity/count cases, %u failures\n",failed?"FAIL":"PASS",failed);
    return failed?1:0;
}
