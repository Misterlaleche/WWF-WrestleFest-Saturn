/* A completed synchronous hardware wait must resume its waiting instruction.
 * The VBlank pulse need not survive a second frontend dispatch/input sample. */
#define main wp1980_launch_only_main
#include "wp1980_launch_test.c"
#undef main

static unsigned idle_level,pulse_level,pulse_pending,wait_calls;
static wf_status_t transient_read(void *u,uint32_t at,uint32_t *out,unsigned n)
{
    if(at==0x140026u&&n==2u){
        unsigned level=pulse_pending?pulse_level:idle_level;
        pulse_pending=0u;
        *out=0xfbffu|(level?0x0400u:0u);
        return WF_OK;
    }
    return read_bus(u,at,out,n);
}
static wf_status_t transient_wait(void *u,wf_r18_frontend_driver_t *d,unsigned level)
{
    (void)u;(void)d;
    ++wait_calls;pulse_level=level;pulse_pending=1u;
    return WF_OK;
}

int main(void)
{
    static const struct {uint32_t waiting,resumed;unsigned level;} cases[]={
        {0x539au,0x53b4u,1u},{0x53cau,0x53ecu,1u},
        {0x5406u,0x5420u,1u},{0x5436u,0x5456u,1u},
        {0x53b4u,0x534au,0u},{0x53ecu,0x534au,0u},
        {0x5420u,0x534au,0u},{0x5456u,0x534au,0u},
        {0x5778u,0x5782u,1u},{0x5782u,0x1e92u,0u}
    };
    wp1980_frontend_input_disable();
    for(unsigned i=0;i<sizeof cases/sizeof cases[0];i++){
        memset(raw,0,sizeof raw);memset(&runtime,0,sizeof runtime);
        wf_r18_frontend_driver_config_t config={.ram=raw,.ram_bytes=sizeof raw,
            .runtime=&runtime,.video=&video,.ssp=0x1c3bd4u,.usp=0x1c3fd4u,.sr=0x2700u,
            .platform={.read_rom=read_rom,.read_bus=transient_read,.store_bus=store_bus,
                .wait_level=transient_wait,.wait_vblank=wait_vblank,.commit=commit,
                .bind_world=bind_world,.submit_records=submit}};
        assert(wf_r18_frontend_driver_init(&frontend,&config)==WF_OK);
        frontend.phase=WF_R18_DRIVER_MODE;frontend.pc=cases[i].waiting;
        frontend.scene.mode.pc=cases[i].waiting;
        frontend.scene.mode.actor_offset=0x5b0u;frontend.regs.d[0]=3u;
        idle_level=!cases[i].level;pulse_pending=wait_calls=0u;
        int status=wf_r18_frontend_driver_step(&frontend);
        if(status!=WF_R18_DRIVER_MORE||frontend.scene.mode.pc!=cases[i].resumed||wait_calls!=1u){
            fprintf(stderr,"FAIL: completed wait %04x must resume %04x; actual %04x status=%d waits=%u\n",
                cases[i].waiting,cases[i].resumed,frontend.scene.mode.pc,status,wait_calls);
            return 1;
        }
    }
    puts("PASS: all 10 mode wait continuations consume the observed hardware level exactly once");
    return 0;
}
