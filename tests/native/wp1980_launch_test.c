/* Host integration probe: real original RESET/coin/START/mode/roster/setup,
 * with a deterministic display-event transport. No participant RAM writes. */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "wp1980_input.h"
#include "wf_gameplay_bindings.h"
#include "wf_r18_match_driver.h"
#include "wf_r18_match_support.h"
#include "wf_r18_frontend_world.h"
#include "wf_r18_runtime_handoff.h"

static uint8_t rom[0x80000],raw[0x4000],scroll[0x20],control[0x20];
static uint16_t backdrop;
static uint32_t clock_count;
static unsigned high,world_mode;
static unsigned host_wait_stride=1u;
static wf_r18_video_t video;
static wf_runtime_t runtime;
static wf_gameplay_binding_context_t gameplay;
static wf_phase_binding_t bindings[48];
static wf_r18_frontend_driver_t frontend;

static uint16_t rw(const uint8_t *p){return (uint16_t)((p[0]<<8)|p[1]);}
static wf_status_t read_rom(void *u,uint32_t at,void *out,size_t n)
{
    (void)u;
    if(at>sizeof rom||n>sizeof rom-at)return WF_BLOCKED;
    memcpy(out,rom+at,n);return WF_OK;
}
static uint16_t port_word(unsigned port)
{
    uint16_t word=port==1u?0xfcffu:0xffffu;
    if(port==3u){word&=(uint16_t)~0x0400u;if(high)word|=0x0400u;}
    return wp1980_frontend_input(frontend.phase,raw,clock_count,port,word);
}
static wf_status_t read_bus(void *u,uint32_t at,uint32_t *out,unsigned n)
{
    (void)u;*out=0;
    for(unsigned i=0;i<n;i++){
        uint32_t a=at+i;uint8_t byte;
        if(a>=0x140020u&&a<0x140028u){uint16_t word=port_word((a-0x140020u)/2u);byte=(uint8_t)(word>>((a&1u)?0:8));}
        else if(a>=0x100000u&&a<0x100020u)byte=scroll[a-0x100000u];
        else if(a>=0x140000u&&a<0x140020u)byte=control[a-0x140000u];
        else if(a>=0x184000u&&a<0x184002u)byte=(uint8_t)(backdrop>>((a&1u)?0:8));
        else return WF_BLOCKED;
        *out=(*out<<8)|byte;
    }
    return WF_OK;
}
static wf_status_t store_bus(void *u,uint32_t at,uint32_t value,unsigned n)
{
    (void)u;uint8_t *p=NULL;
    if(at>=0x100000u&&at+n<=0x100020u)p=scroll+at-0x100000u;
    else if(at>=0x140000u&&at+n<=0x140020u)p=control+at-0x140000u;
    else if(at==0x184000u&&n==2u){backdrop=(uint16_t)value;return WF_OK;}
    if(!p)return WF_BLOCKED;
    for(unsigned i=0;i<n;i++)p[i]=(uint8_t)(value>>(8u*(n-i-1u)));
    return WF_OK;
}
static wf_status_t tick(wf_r18_frontend_driver_t *d)
{
    for(unsigned elapsed=0;elapsed<host_wait_stride;elapsed++){
        high=1u;++clock_count;
        if(clock_count>20000u){fprintf(stderr,"clock limit pc=%x phase=%u\n",d->pc,d->phase);exit(2);}
        for(unsigned i=0;i<17u;i++)if((d->control.sr&0x0700u)<0x0200u)
            if(wf_r18_frontend_driver_on_raster(d)!=WF_OK)return WF_BLOCKED;
        if((d->control.sr&0x0700u)<0x0300u)
            if(wf_r18_frontend_driver_on_vblank(d)!=WF_OK)return WF_BLOCKED;
    }
    return WF_OK;
}
static wf_status_t wait_level(void *u,wf_r18_frontend_driver_t *d,unsigned level)
{
    (void)u;
    if(level&&!high)return tick(d);
    high=level;return WF_OK;
}
static wf_status_t wait_vblank(void *u,wf_r18_frontend_driver_t *d)
{(void)u;high=0;return tick(d);}
static wf_status_t commit(void *u,wf_r18_frontend_driver_t *d)
{(void)u;(void)d;return WF_OK;}
static wf_status_t bind_world(void *u,unsigned mode)
{(void)u;world_mode=mode;return WF_OK;}
static wf_status_t submit(void *u,const wf_r18_sprite_record_t *records,size_t count)
{(void)u;(void)records;return count<=512u?WF_OK:WF_BLOCKED;}
static wf_status_t support_rom(void *u,uint32_t at,uint8_t *out,size_t n)
{return read_rom(u,at,out,n);}
static wf_status_t support_palette(void *u,uint32_t at,uint16_t value)
{(void)u;wf_r17_fg0_write_t write={at,value,2};return wf_r18_video_batch(&runtime,&write,1,&video);}
static wf_status_t extension(void *u,uint32_t owner,wf_r18_prematch_t *m)
{
    (void)u;
    if(owner==0x285dau){
        wf_r18_frontend_world_t world={.ram=m->ram,.ram_bytes=0x4000u,.regs=m->regs,.a=m->a,.read_rom=read_rom};
        wf_status_t status=wf_r18_frontend_world_call(&world,owner);
        if(status!=WF_OK)m->dependency=world.blocked_dependency;
        return status;
    }
    wf_r18_match_support_t support={.machine=m,.fg_0c0000=video.fg0,.fg_bytes=sizeof video.fg0,
        .read_rom=support_rom,.write_palette=support_palette};
    return wf_r18_match_support_call(&support,owner);
}
int main(int argc,char **argv)
{
    assert(argc==2);FILE *file=fopen(argv[1],"rb");assert(file);
    assert(fread(rom,1,sizeof rom,file)==sizeof rom);assert(!fclose(file));
    size_t count=wf_gameplay_bindings_build(&gameplay,bindings,48);
    assert(count);wf_runtime_init(&runtime,bindings,count);
    const char *seed_env=getenv("WP1980_HOST_SEED"),*stride_env=getenv("WP1980_HOST_WAIT_STRIDE");
    uint32_t seed=seed_env?(uint32_t)strtoul(seed_env,NULL,0):1085u;
    host_wait_stride=stride_env?(unsigned)strtoul(stride_env,NULL,0):1u;
    assert(host_wait_stride>=1u&&host_wait_stride<=16u);
    for(unsigned i=0;i<4u;i++)raw[0x5cu+i]=(uint8_t)(seed>>(24u-8u*i));
    wf_r18_frontend_driver_config_t config={.ram=raw,.ram_bytes=sizeof raw,
        .runtime=&runtime,.video=&video,.ssp=0x1c3bd4u,.usp=0x1c3fd4u,.sr=0x2700u,
        .platform={.read_rom=read_rom,.read_bus=read_bus,.store_bus=store_bus,
            .wait_level=wait_level,.wait_vblank=wait_vblank,.commit=commit,
            .bind_world=bind_world,.submit_records=submit}};
    assert(wf_r18_frontend_driver_init(&frontend,&config)==WF_OK);
    unsigned steps=0;
    while(frontend.phase==WF_R18_DRIVER_RESET||
          (frontend.phase==WF_R18_DRIVER_BOOT&&
           !(frontend.scene.boot.pc==0x1e7cu&&frontend.scene.boot.hold_return==0x78au))){
        int status=wf_r18_frontend_driver_step(&frontend);
        if(status<0){fprintf(stderr,"boot blocked pc=%x dep=%x\n",frontend.pc,frontend.blocked_dependency);return 2;}
        assert(++steps<10000u);
    }
    wp1980_frontend_input_begin(clock_count);
    if(wf_r18_frontend_driver_run(&frontend)!=WF_OK){fprintf(stderr,"select blocked pc=%x dep=%x\n",frontend.pc,frontend.blocked_dependency);return 2;}
    assert(frontend.phase==WF_R18_DRIVER_AC0);
    assert(raw[0x161]&1u);assert(rw(raw+0x7c)==0x8000u);
    assert(rw(raw+0x5b0u+0x56u)==0u);
    wf_r18_match_driver_t match;
    assert(wf_r18_match_driver_init(&match,&frontend,extension,NULL)==WF_OK);
    if(wf_r18_match_driver_run(&match)!=WF_OK){fprintf(stderr,"match blocked pc=%x dep=%x front=%x/%x\n",match.blocked_pc,match.blocked_dependency,frontend.pc,frontend.blocked_dependency);return 2;}
    assert(wf_r18_runtime_handoff_f8c(&runtime,raw,sizeof raw,&frontend.regs)==WF_OK);
    unsigned active=0;
    for(unsigned i=0;i<WF_ACTOR_SLOTS;i++){
        const wf_actor_raw_t *a=&runtime.actors[i];
        if(!(wf_u8(a,WF_A_ACTIVE)&0x80u))continue;
        printf("slot=%u id=%u x=%u y=%u role=%02x health=%u\n",i,
            wf_be16(a,WF_A_CHARACTER),wf_be16(a,WF_A_X),wf_be16(a,WF_A_Y),
            wf_u8(a,WF_A_ROLE),wf_be16(a,WF_A_STAMINA));
        ++active;
    }
    assert(active==6u);assert(runtime.g.mode_0161&1u);assert(runtime.g.credited_route==0x8000u);
    assert(runtime.frame_stage==3u);assert(runtime.g.frame_counter==0u);assert(world_mode==0u);
    assert((wf_u8(&runtime.actors[0],WF_A_ROLE)&0x80u)==0u);
    for(unsigned i=4u;i<9u;i++)assert(wf_u8(&runtime.actors[i],WF_A_ROLE)&0x80u);
    wf_runtime_t before=runtime;
    (void)wp1980_input(&runtime,0xffu);assert(!memcmp(&runtime,&before,sizeof runtime));
    assert(wp1980_input(&runtime,0xfeu)==0xfeu);assert(!wp1980_demo_active);
    assert(wp1980_input(&runtime,0xffu)==0xffu);
    printf("PASS: ordinary input reaches credited Royal;6 original actors;F8C handoff;physical control takeover;clock=%u;seed=%u;wait_stride=%u;game_rng=%08x\n",clock_count,seed,host_wait_stride,runtime.g.rng_state);
    return 0;
}
