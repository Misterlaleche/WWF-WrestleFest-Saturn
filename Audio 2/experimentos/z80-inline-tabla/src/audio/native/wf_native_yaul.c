#include "wf_native_yaul.h"
#include <smpc/smc.h>
static bool sound_enable(bool enable,void *user)
{
    (void)user;
    (void)smpc_smc_call(enable ? SMPC_SMC_SNDON : SMPC_SMC_SNDOFF);
    return true; /* Yaul's call waits for SMPC completion. */
}
static void write16(uint32_t a,uint16_t v,void *user)
{
    (void)user;
    *(volatile uint16_t *)(uintptr_t)(0x25a00000u+a)=v;
}
static void barrier(void *user)
{
    volatile uint16_t drain;
    (void)user;
    drain=*(volatile uint16_t *)(uintptr_t)0x25a00000u;
    (void)drain;
}
void wf_native_yaul_complete_platform(wf_native_platform_t *o)
{
    if(o) {
        o->sound_enable=sound_enable;
        o->write16=write16;
        o->barrier=barrier;
    }
}
