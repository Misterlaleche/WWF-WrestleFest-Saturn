/* Optional full-native host fixture for WF_JO_ASSETS_TEST_WORKER.
 * Runs the private job at join so native gates exercise the paired ABI without
 * requiring Jo's hardware slave queue. Real pthread interleaving is covered by
 * test_assets_paired.c. Never link this fixture into a Saturn image. */
#include "wf_jo_assets.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#if !defined(WF_JO_ASSETS_HOST) || !defined(WF_JO_ASSETS_TEST_WORKER)
#error "Host asset worker fixture requires both explicit test defines"
#endif
static void (*pending)(void);
unsigned wf_test_assets_worker_submitted,wf_test_assets_worker_joined;

static void worker_report(void)
{
    const char *path=getenv("WF_JO_TEST_WORKER_STATS");
    if(path){
        FILE *out=fopen(path,"w");
        if(!out){perror(path);abort();}
        fprintf(out,"{\"enabled\":true,\"schedule\":\"deferred-at-join\","
            "\"submitted\":%u,\"joined\":%u,\"pending\":%u}\n",
            wf_test_assets_worker_submitted,wf_test_assets_worker_joined,pending!=NULL);
        if(fclose(out)){perror(path);abort();}
    }
    printf("JO_ASSETS_WORKER submitted=%u joined=%u pending=%u\n",
        wf_test_assets_worker_submitted,wf_test_assets_worker_joined,pending!=NULL);
    assert(!pending&&wf_test_assets_worker_submitted==wf_test_assets_worker_joined);
}
__attribute__((constructor)) static void worker_init(void)
{assert(!atexit(worker_report));}

void wf_jo_assets_test_submit(void (*callback)(void))
{
    assert(callback&&!pending);pending=callback;++wf_test_assets_worker_submitted;
}
void wf_jo_assets_test_join(void)
{
    assert(pending);pending();pending=0;++wf_test_assets_worker_joined;
}
