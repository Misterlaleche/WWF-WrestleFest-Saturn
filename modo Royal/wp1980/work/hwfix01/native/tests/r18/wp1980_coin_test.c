#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "wp1980_input.h"
#include "wf_r18_frontend_coin.h"
#include "wf_r18_frontend_driver.h"

static uint32_t display_clock;
static int cabinet(void *u,wf_r18_title_event_t *event,wf_r18_frontend_control_context_t *c)
{
    (void)u;
    if(event->kind==WF_R18_CONTROL_READ16){
        assert(event->address==0x140020u);
        event->value=wp1980_frontend_input(WF_R18_DRIVER_BOOT,c->ram_1c0000,
            display_clock,0u,0xffffu);
    }
    return 0;
}
int main(void)
{
    uint8_t raw[0x4000]={0};
    raw[0x10]=4u;
    wp1980_frontend_input_begin(100u);
    /* A load takes ten display periods before the first cabinet read. */
    uint16_t word=wp1980_frontend_input(WF_R18_DRIVER_BOOT,raw,110u,0u,0xffffu);
    assert(!(word&0x0100u));

    memset(raw,0,sizeof raw);raw[0x4b]=raw[0x4d]=1u;
    wf_r17_m68k_shadow_t regs={.valid=1};uint32_t address[7]={0};
    wf_r18_frontend_control_context_t context={.ram_1c0000=raw,.ram_bytes=sizeof raw,
        .regs=&regs,.a=address,.ssp=0x1c3bd4u,.usp=0x1c3fd4u,.sr=0x2200u,.effect=cabinet};
    wp1980_frontend_input_begin(100u);display_clock=110u;
    for(unsigned poll=0;poll<120u;poll++){
        wf_r18_frontend_coin_t coin;
        context.ssp-=6u;context.sr=0x2200u;
        wf_r18_frontend_coin_enter(&coin,0x2000u,0x1e7cu);
        int status;
        do{status=wf_r18_frontend_coin_step(&coin,&context);}while(status==WF_R18_CONTROL_MORE);
        assert(status==WF_R18_CONTROL_RTE);
        /* Arbitrary physical display gaps cannot erase a polled input. */
        if(poll%7u==0u)display_clock+=11u;
    }
    assert(raw[0x4f]==1u);
    word=wp1980_frontend_input(WF_R18_DRIVER_BOOT,raw,display_clock,0u,0xffffu);
    assert(word&0x0100u);assert(!(word&0x0080u));
    raw[0x4f]=0u; /* Original START consumes the earned credit. */
    word=wp1980_frontend_input(WF_R18_DRIVER_BOOT,raw,display_clock+100u,0u,0xffffu);
    assert(word&0x0100u);assert(!(word&0x0080u));
    wp1980_frontend_input_disable();
    assert(wp1980_frontend_input(WF_R18_DRIVER_BOOT,raw,display_clock,0u,0xffffu)==0xffffu);
    puts("PASS: delayed first read and arbitrary display stalls produce exactly one original credit and START");
    return 0;
}
