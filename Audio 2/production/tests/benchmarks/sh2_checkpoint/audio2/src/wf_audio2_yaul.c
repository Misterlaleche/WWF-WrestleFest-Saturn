#include <yaul.h>
#include "wf_audio2_yaul.h"
#include "wf_music_module.h"
extern uint8_t wf_hotcode_start[],wf_hotcode_end[];
extern void wf_music_module_prepare(void),wf_music_module_run(void);
static volatile uint16_t exited __attribute__((section(".uncached.audio2_worker")))=1;
static void worker_entry(void)
{
    void (*run)(void)=(void (*)(void))((uintptr_t)wf_music_module_run|0x20000000u);
    run();
    /* Publication happens after every instruction in the audio worker,
     * including its failure cleanup. A state==FAULT alone is insufficient. */
    exited=1;
}
bool wf_audio2_yaul_worker_quiescent(void *user)
{(void)user;return exited!=0;}
bool wf_audio2_yaul_worker_start(uint32_t workspace,void *user)
{
    (void)user;
    if(!exited)return false;
    volatile wf_music_module_shared_t *s=(void *)WF_MUSIC_MODULE_SHARED;
    s->workspace=workspace;wf_music_module_prepare();
    if(s->state!=WF_MUSIC_BOOTING)return false;
    cpu_cache_area_purge(wf_hotcode_start,(size_t)(wf_hotcode_end-wf_hotcode_start)+3220u);
    cpu_dual_slave_clear();cpu_dual_comm_mode_set(CPU_DUAL_ENTRY_POLLING);
    exited=0;
    cpu_dual_slave_set((cpu_dual_slave_entry_t)((uintptr_t)worker_entry|0x20000000u));
    cpu_dual_slave_notify();return true;
}
