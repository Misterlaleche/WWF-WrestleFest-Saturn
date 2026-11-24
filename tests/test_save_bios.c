/* Dedicated test disc, never linked into the game. Use only with a fresh
 * private emulator profile: scenarios 1/2 create test files through the BIOS.
 * No guest memory injection, savestate or replacement storage backend. */
#include <jo/jo.h>
#include <string.h>
#include "wf_jo_save.h"
#include "save_fixture.h"
#ifndef SAVE_SCENARIO
#define SAVE_SCENARIO 0
#endif

/* Same cache end as the game; follow its complete-slot lease size. */
#define WORKSPACE ((void *)(0x002fc000u-WF_JO_SAVE_WORKSPACE_BYTES))
volatile uint32_t wf_save_probe[12];
uint8_t wf_save_probe_rankings[WF_R18_SAVE_RANKING_BYTES];
static wf_jo_save_t context;

static void run_probe(void)
{
    uint8_t before[WF_R18_SAVE_RANKING_BYTES];
    uint8_t image[WF_R18_SAVE_IMAGE_BYTES],readback[WF_R18_SAVE_IMAGE_BYTES];
    memcpy(wf_save_probe_rankings,save_factory,sizeof save_factory);
    memcpy(before,wf_save_probe_rankings,sizeof before);
    wf_jo_save_init(&context,NULL,NULL);
    wf_save_probe[1]=1;
    wf_save_probe[2]=SAVE_SCENARIO;
    if(SAVE_SCENARIO==1 || SAVE_SCENARIO==2){
        memset(image,0x5a,sizeof image);
        if(!context.backend->open(context.user,WORKSPACE,WF_JO_SAVE_WORKSPACE_BYTES)){
            wf_save_probe[10]=1;goto done;
        }
        if(SAVE_SCENARIO==1){
            if(!context.backend->write(context.user,"WFESTSAT001",image,sizeof image))
                wf_save_probe[10]=2;
        }else{
            /* Exhaust only this disposable profile, using ordinary BIOS writes. */
            for(unsigned i=0;i<1000u;++i){
                char name[12]="WFTEST00000";
                name[10]=(char)('0'+i%10u);name[9]=(char)('0'+i/10u%10u);
                name[8]=(char)('0'+i/100u%10u);
                if(!context.backend->write(context.user,name,image,sizeof image))break;
                ++wf_save_probe[9];
            }
        }
        context.backend->close(context.user);
        if(wf_save_probe[10])goto done;
    }
    wf_save_probe[1]=2;
    wf_save_probe[3]=wf_jo_save_load(&context,wf_save_probe_rankings,WORKSPACE,WF_JO_SAVE_WORKSPACE_BYTES);
    wf_save_probe[5]=!memcmp(before,wf_save_probe_rankings,sizeof before);
    wf_save_probe[6]=!memcmp(save_expected,wf_save_probe_rankings,sizeof save_expected);
    if(wf_save_probe[3]==WF_JO_SAVE_DEFAULTS || SAVE_SCENARIO==1){
        memcpy(wf_save_probe_rankings,save_expected,sizeof save_expected);
        wf_save_probe[7]=wf_jo_save_needs_store(&context,wf_save_probe_rankings);
        wf_save_probe[1]=3;
        wf_save_probe[4]=wf_jo_save_store(&context,wf_save_probe_rankings,WORKSPACE,WF_JO_SAVE_WORKSPACE_BYTES);
    }else wf_save_probe[4]=context.status;
    if(SAVE_SCENARIO==1){
        if(context.backend->open(context.user,WORKSPACE,WF_JO_SAVE_WORKSPACE_BYTES)){
            wf_save_probe[8]=context.backend->read(context.user,"WFESTSAT001",readback,sizeof readback)
                &&!memcmp(image,readback,sizeof image);
            context.backend->close(context.user);
        }
    }
done:
    wf_save_probe[1]=4;
    jo_printf(0,2,"SAVE BIOS %d %d",wf_save_probe[3],wf_save_probe[4]);
}
void jo_main(void)
{
    wf_save_probe[0]=0x57425331u; /* WBS1 */
    jo_core_init(JO_COLOR_Black);
    run_probe();
    jo_core_run();
}
