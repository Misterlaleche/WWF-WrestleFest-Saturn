/* Host integration probe: real original RESET/coin/START/mode/roster/setup,
 * with a deterministic display-event transport. No participant RAM writes. */
#include <assert.h>
#include "assets_fixture.h"
#include "wf_r18_present.h"
#include "wf_r18_ring.h"
#include "wf_r18_hud.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "wp1980_input.h"
#include "wf_gameplay_bindings.h"
#include "wf_r18_match_driver.h"
#include "wf_r18_match_support.h"
#include "wf_r18_frontend_world.h"
#include "wf_r18_runtime_handoff.h"
#include "wf_r18_post_tag.h"

static uint8_t rom[0x80000],raw[0x4000],scroll[0x20],control[0x20];
static uint16_t backdrop;
static uint32_t clock_count;
static unsigned high,world_mode;
static int wanted_royal;
static unsigned credits_active,credits_skip;
static uint8_t scene_blob[131072];
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
    return wf_r18_rom_read(NULL,at,out,n);
}
/* Ordinary input fixture. It changes pad/coin levels only; no actor,
 * selection, RNG or timer state is manufactured. A is never held to skip
 * prematch. Before the title entrance completes, all inputs stay released. */
static uint16_t selected_input(unsigned phase,const uint8_t *ram,uint32_t clock,
 unsigned port,uint16_t physical){
 if(credits_active){
  if(!port&&((credits_skip==1u&&frontend.pc==0x1c34u)||
              (credits_skip==2u&&frontend.pc==0x1d1au)))physical&=(uint16_t)~0x10u;
  return physical;
 }
 if(phase==WF_R18_DRIVER_AC0)return physical;
 if(!wanted_royal&&phase==WF_R18_DRIVER_MODE){
  uint16_t word=(uint16_t)(physical|0xffu);if(port)return word;
  if(ram[0x161]&1u)word&=(uint16_t)~1u;else word&=(uint16_t)~0x10u;
  return word;
 }
 if(!wanted_royal&&phase==WF_R18_DRIVER_ROSTER){
  uint16_t word=(uint16_t)(physical|0xffu);
  if(!port&&(clock&7u)<4u)word&=(uint16_t)~0x10u;
  return word;
 }
 return wp1980_frontend_input(phase,ram,clock,port,physical);
}
static uint16_t port_word(unsigned port)
{
    uint16_t word=port==1u?0xfcffu:0xffffu;
    if(port==3u){word&=(uint16_t)~0x0400u;if(high)word|=0x0400u;}
    return selected_input(frontend.phase,raw,clock_count,port,word);
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
    if(at==0x14000cu&&n==2u)printf("FRONTEND_AUDIO clock=%u command=%04x\n",clock_count,(unsigned)value);
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
        high=1u;++clock_count;wf_host_assets_frame(clock_count);
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
{(void)u;
 if(wf_r18_hud_commit(&video)!=WF_OK)return WF_BLOCKED;
 for(unsigned role=0;role<4u;role++)if(wf_r18_palettes_commit(&video,role)!=WF_OK)return WF_BLOCKED;
 if(d->world_visible&&wf_r18_ring_update((int16_t)rw(scroll),(int16_t)(2048u-rw(scroll+2)),
     (int16_t)rw(scroll+4),(int16_t)(2048u-rw(scroll+6)),(uint8_t)rw(control+0x10)))return WF_BLOCKED;
 return WF_OK;}
static wf_status_t bind_world(void *u,unsigned mode)
{(void)u;world_mode=mode;
 const char *names[7]={"R18RING.BIN","R18MAP1.BIN","R18MAP2.BIN","R18MENU.BIN","R18MAP4.BIN","R18MAP5.BIN","R18MAP6.BIN"};
 uint32_t bytes;if(mode>=7u||wf_jo_assets_file_size(names[mode],&bytes)||bytes>sizeof scene_blob||
 wf_jo_assets_file_read(names[mode],0,scene_blob,bytes)||wf_r18_ring_bind(scene_blob,bytes))return WF_BLOCKED;
 wf_r18_video_ring_bind(&video);printf("FRONTEND_MAP clock=%u mode=%u\n",clock_count,mode);return WF_OK;}
static wf_status_t submit(void *u,const wf_r18_sprite_record_t *records,size_t count)
{(void)u;size_t capacity;
 wf_r18_sprite_record_t *out=wf_r18_present_records_buffer(&capacity);
 if(count>capacity)return WF_BLOCKED;
 if(count)memcpy(out,records,count*sizeof *records);
 wf_status_t status=wf_r18_present_records_commit(&video,count);
 if(status!=WF_OK)fprintf(stderr,"frontend render blocked clock=%u count=%zu commands=%u\n",clock_count,count,wf_r18_present_commands());
 return status;}
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
static void check_credits(wf_r18_match_driver_t *match,unsigned skip)
{
    wf_r18_post_tag_t cursor;wf_r18_post_tag_init(&cursor,0x1b4e);
    wf_r18_post_tag_context_t context={&match->machine,read_rom,NULL};
    unsigned pages=0,staff=0,movements=0,levels=0,requests=0,start=clock_count;
    /* This isolated owner test starts from a real prematch handoff. Credits
     * overwrites six staff objects but its original scanner also visits any
     * still-active tail objects; do not manufacture a match-win RAM image. */
    unsigned staff_objects=6;
    for(unsigned slot=6;slot<9u;slot++)staff_objects+=!!(raw[0x5b0u+slot*0x10cu]&128u);
    credits_active=1;credits_skip=skip;
    for(;;){
        frontend.pc=cursor.pc;int result=wf_r18_post_tag_step(&cursor,&context);
        if(result==WF_R18_POST_MORE)continue;
        if(result==WF_R18_POST_BOUNDARY){assert(cursor.pc==0x6fcu);break;}
        if(result!=WF_R18_POST_REQUEST){
            fprintf(stderr,"credits blocked pc=%x dep=%x result=%d\n",cursor.pc,cursor.blocked_dependency,result);exit(2);
        }
        ++requests;uint16_t value=0;wf_status_t status=WF_BLOCKED;
        wf_r18_post_tag_request_t *q=&cursor.request;
        if(q->kind==WF_R18_POST_CALL){
            if(q->owner==0x2503cu)++pages;
            if(q->owner==0x2208u)++movements;
            status=wf_r18_match_driver_owner(match,q->owner);
        }else if(q->kind==WF_R18_POST_READ16){
            if(q->call_pc==0x1d1au)++staff;
            uint32_t word;status=read_bus(NULL,q->address,&word,2u);value=(uint16_t)word;
        }else if(q->kind==WF_R18_POST_WRITE16)status=store_bus(NULL,q->address,q->value,2u);
        else if(q->kind==WF_R18_POST_LEVEL||q->kind==WF_R18_POST_SYNC){
            memcpy(frontend.a,match->machine.a,sizeof frontend.a);
            if(q->kind==WF_R18_POST_LEVEL){++levels;status=wait_level(NULL,&frontend,q->value);}
            else status=wait_vblank(NULL,&frontend);
            memcpy(match->machine.a,frontend.a,sizeof frontend.a);
        }
        if(status!=WF_OK){fprintf(stderr,"credits dependency pc=%x owner=%x blocked=%x/%x front=%x/%x\n",cursor.pc,q->owner,match->blocked_pc,match->blocked_dependency,frontend.pc,frontend.blocked_dependency);exit(2);}
        assert(wf_r18_post_tag_complete(&cursor,&context,status,value)==WF_R18_POST_MORE);
    }
    assert(pages==(skip==1u?1u:9u));assert(staff==(skip==1u?0u:skip==2u?1u:1281u));
    assert(movements==staff*staff_objects);assert(world_mode==4u);assert(raw[0x76]&128u);
    memcpy(frontend.a,match->machine.a,sizeof frontend.a);
    assert(wf_r18_frontend_driver_resume_boot(&frontend,0x6fcu)==WF_OK);
    assert(frontend.phase==WF_R18_DRIVER_BOOT);
    printf("PASS: native Credits 1B4E -> ranking -> 6FC;mode=%s;skip=%u;pages=%u;staff=%u;movements=%u;levels=%u;requests=%u;elapsed=%u\n",
           wanted_royal?"Royal":"TAG",skip,pages,staff,movements,levels,requests,clock_count-start);
}
static void check_royal_loss(wf_r18_match_driver_t *match,unsigned initial_high)
{
    assert(initial_high<2u);high=initial_high;credits_active=1;credits_skip=0;
    wf_r18_post_tag_t cursor;wf_r18_post_tag_init(&cursor,0x1afcu);
    wf_r18_post_tag_context_t context={&match->machine,read_rom,NULL};
    unsigned start=clock_count,calls=0;
    const uint32_t expected[]={0x2a06u,0x2503cu,0x21e6u,0x65c0u};
    for(;;){
        frontend.pc=cursor.pc;int result=wf_r18_post_tag_step(&cursor,&context);
        if(result==WF_R18_POST_MORE)continue;
        if(result==WF_R18_POST_BOUNDARY){assert(cursor.pc==0x6fcu);break;}
        assert(result==WF_R18_POST_REQUEST&&cursor.request.kind==WF_R18_POST_CALL);
        assert(calls<4u&&cursor.request.owner==expected[calls++]);
        wf_status_t status=wf_r18_match_driver_owner(match,cursor.request.owner);
        assert(status==WF_OK);
        assert(wf_r18_post_tag_complete(&cursor,&context,status,0)==WF_R18_POST_MORE);
    }
    assert(calls==4u&&clock_count-start==98u-initial_high&&!high);
    printf("PASS: Royal loss 1AFC -> 2A06 -> text -> 97 periods -> ranking -> 6FC;initial_high=%u;elapsed=%u\n",initial_high,clock_count-start);
}
int main(int argc,char **argv)
{
    assert(argc==3);wanted_royal=atoi(argv[2]);
    wf_host_assets_init();wf_r18_present_init(wf_jo_assets_file_read,NULL,0,NULL,0);
    assert(wf_r18_hud_bind(&video,wf_jo_assets_file_read)==WF_OK);
    wf_host_assets_play();wf_r18_title_portrait_divisor_set(1u);wf_r18_prematch_speech_divisor_set(1u);
    FILE *file=fopen(argv[1],"rb");assert(file);
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
    while(frontend.phase!=WF_R18_DRIVER_TITLE||frontend.scene.title.pc!=0x837eu){
        int status=wf_r18_frontend_driver_step(&frontend);
        if(status<0){fprintf(stderr,"boot blocked pc=%x dep=%x\n",frontend.pc,frontend.blocked_dependency);return 2;}
        assert(++steps<10000u);
    }
    printf("FRONTEND_COMPLETE_INTRO clock=%u\n",clock_count);wp1980_frontend_input_begin(clock_count);
    if(wf_r18_frontend_driver_run(&frontend)!=WF_OK){fprintf(stderr,"select blocked pc=%x dep=%x\n",frontend.pc,frontend.blocked_dependency);return 2;}
    assert(frontend.phase==WF_R18_DRIVER_AC0);
    assert(!!(raw[0x161]&1u)==!!wanted_royal);assert(rw(raw+0x7c)==0x8000u);
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
    assert(active==(wanted_royal?6u:4u));assert(!!(runtime.g.mode_0161&1u)==!!wanted_royal);assert(runtime.g.credited_route==0x8000u);
    assert(runtime.frame_stage==3u);assert(runtime.g.frame_counter==0u);assert(world_mode==0u);
    assert((wf_u8(&runtime.actors[0],WF_A_ROLE)&0x80u)==0u);
    for(unsigned i=4u;i<WF_ACTOR_SLOTS;i++)if(wf_u8(&runtime.actors[i],WF_A_ACTIVE)&128u)assert(wf_u8(&runtime.actors[i],WF_A_ROLE)&0x80u);
    wf_runtime_t before=runtime;
    (void)wp1980_input(&runtime,0xffu);assert(!memcmp(&runtime,&before,sizeof runtime));
    assert(wp1980_input(&runtime,0xfeu)==0xfeu);assert(!wp1980_demo_active);
    assert(wp1980_input(&runtime,0xffu)==0xffu);
    printf("PASS: complete original intro -> selection -> prematch -> F8C;mode=%s;actors=%u;clock=%u;seed=%u;wait_stride=%u;game_rng=%08x\n",wanted_royal?"Royal":"TAG",active,clock_count,seed,host_wait_stride,runtime.g.rng_state);
    const char *credits=getenv("WF_JO_TEST_CREDITS");
    if(credits)check_credits(&match,(unsigned)strtoul(credits,NULL,0));
    const char *loss=getenv("WF_JO_TEST_ROYAL_LOSS");
    if(loss){assert(!credits);check_royal_loss(&match,(unsigned)strtoul(loss,NULL,0));}
    return 0;
}
