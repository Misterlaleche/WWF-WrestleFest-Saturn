/* Jo transport for the original native cabinet owners. No game state machine
 * or scene duration is duplicated here; waits deliver admitted arcade IRQs. */
#include "wf_jo_frontend.h"
#include "wf_jo_assets.h"
#include "wf_jo_scene_prepare.h"
#include "wf_jo_audio.h"
#include "wf_jo_video.h"
#include "wf_r18_frontend_world.h"
#include "wf_r18_hud.h"
#include "wf_r18_match_support.h"
#include "wf_r18_post_tag.h"
#include "wf_r18_present.h"
#include "wf_r18_referee.h"
#include "wf_r18_ring.h"
#include "wf_r18_runtime_handoff.h"
#include <string.h>
#ifdef WF_JO_FRONTEND_TEST
#include "wf_jo_frontend_test_platform.h"
#else
#include <jo/jo.h>
#endif

extern void __jo_core_synch(void);
volatile uint32_t wf_jo_front_diagnostic[8];
#if defined(JO_COMPILE_WITH_PROFILER_SUPPORT) && !defined(WF_JO_FRONTEND_TEST)
/* Calls, admitted IRQ2s, FRT total/max, milliseconds total/max, physical
 * snapshots, last FRT. FRT intervals wrap at65536; ms exposes a long call. */
volatile uint32_t wf_jo_coin_profile[8];
#endif
/* Each admitted frontend IRQ batch or combat callback samples physical pads
 * once. Every original IRQ and optional input callback still executes. */
static const wf_jo_frontend_t *sampled_controls_owner;
static uint8_t sampled_controls[4];
static const char *const world_names[7]={"R18RING.BIN","R18MAP1.BIN","R18MAP2.BIN",
                                       "R18MENU.BIN","R18MAP4.BIN","R18MAP5.BIN","R18MAP6.BIN"};

static void observe(const wf_jo_frontend_t *f)
{
    wf_jo_front_diagnostic[0]=f->driver.phase;wf_jo_front_diagnostic[1]=f->driver.pc;
    wf_jo_front_diagnostic[2]=f->arcade_ticks;wf_jo_front_diagnostic[3]=jo_core_get_vblank_count();
    wf_jo_front_diagnostic[4]=f->raster_irqs;wf_jo_front_diagnostic[5]=f->vblank_irqs;
    wf_jo_front_diagnostic[6]=f->frame_commits;wf_jo_front_diagnostic[7]=f->config.clock->pending;
}

static uint16_t read16(const uint8_t *p)
{return (uint16_t)((uint16_t)p[0]<<8|p[1]);}
static uint32_t read32(const uint8_t *p)
{return (uint32_t)read16(p)<<16|read16(p+2);}
static void write16(uint8_t *p,uint16_t value)
{p[0]=(uint8_t)(value>>8);p[1]=(uint8_t)value;}
static int overlaps(const void *a,size_t an,const void *b,size_t bn)
{
    uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;
    return an>UINTPTR_MAX-x||bn>UINTPTR_MAX-y||(x<y+bn&&y<x+an);
}
static wf_status_t fault(wf_jo_frontend_t *f,uint32_t pc,uint32_t dependency)
{
    if(!f->blocked_dependency){f->blocked_pc=pc;f->blocked_dependency=dependency?dependency:pc;}
    return WF_BLOCKED;
}
static wf_status_t rom_read(void *user,uint32_t at,void *out,size_t bytes)
{(void)user;return wf_r18_rom_read(NULL,at,out,bytes);}

static uint8_t controls(unsigned player)
{
    uint8_t level=0xffu;
    int input=jo_get_player_input_port((int)player);
    if(input<0)return level;
    unsigned port=(unsigned)input;
    if(jo_is_input_key_pressed(port,JO_KEY_RIGHT))level&=(uint8_t)~1u;
    if(jo_is_input_key_pressed(port,JO_KEY_LEFT))level&=(uint8_t)~2u;
    /* Original cabinet direction bits, shared with combat. */
    if(jo_is_input_key_pressed(port,JO_KEY_UP))level&=(uint8_t)~4u;
    if(jo_is_input_key_pressed(port,JO_KEY_DOWN))level&=(uint8_t)~8u;
    if(jo_is_input_key_pressed(port,JO_KEY_A))level&=(uint8_t)~16u;
    if(jo_is_input_key_pressed(port,JO_KEY_B))level&=(uint8_t)~32u;
    if(jo_is_input_key_pressed(port,JO_KEY_C))level&=(uint8_t)~64u;
    if(jo_is_input_key_pressed(port,JO_KEY_START))level&=(uint8_t)~128u;
    return level;
}
static inline __attribute__((always_inline)) uint16_t finish_input_word(wf_jo_frontend_t *f,unsigned port,uint16_t word)
{
    if(port==3u){word&=(uint16_t)~0x0400u;if(f->high)word|=0x0400u;}
    if(f->config.input_word)word=f->config.input_word(f->config.user,f,port,word);
    return word;
}
static __attribute__((noinline)) uint16_t input_word_fallback(wf_jo_frontend_t *f,unsigned port)
{
    uint8_t level=sampled_controls_owner==f?sampled_controls[port]:controls(port);
    uint16_t word=port==1u?0xfcffu:0xffffu;
    word=(uint16_t)((word&0xff00u)|level);
    if(!port){
        if(sampled_controls_owner==f){
            if(!(sampled_controls[0]&64u)||!(sampled_controls[2]&64u))word&=(uint16_t)~0x0100u;
            if(!(sampled_controls[1]&64u)||!(sampled_controls[3]&64u))word&=(uint16_t)~0x0200u;
        }else{
            if(!(level&64u)||!(controls(2)&64u))word&=(uint16_t)~0x0100u;
            if(!(controls(1)&64u)||!(controls(3)&64u))word&=(uint16_t)~0x0200u;
        }
    }
    return finish_input_word(f,port,word);
}
static uint16_t input_word(wf_jo_frontend_t *f,unsigned port)
{
    /* The usual IRQ batch already has its physical pad snapshot. Its word
     * construction contains no calls before the optional input callback;
     * the unsampled fallback's controls() calls must not enlarge this frame.
     * Still invoke the callback separately for every original READ16. */
    if(sampled_controls_owner!=f)return input_word_fallback(f,port);
    uint16_t word=(uint16_t)((port==1u?0xfc00u:0xff00u)|sampled_controls[port]);
    if(!port){
        if(!(sampled_controls[0]&64u)||!(sampled_controls[2]&64u))word&=(uint16_t)~0x0100u;
        if(!(sampled_controls[1]&64u)||!(sampled_controls[3]&64u))word&=(uint16_t)~0x0200u;
    }
    return finish_input_word(f,port,word);
}
static __attribute__((noinline)) wf_status_t bus_read_bytes(wf_jo_frontend_t *f,uint32_t at,uint32_t *out,unsigned width)
{
    uint32_t value=0;
    for(unsigned i=0;i<width;i++){
        uint32_t address=at+i;uint8_t byte;
        if(address>=0x140020u&&address<0x140028u){
            uint16_t word=input_word(f,(address-0x140020u)/2u);
            byte=(uint8_t)(word>>((address&1u)?0:8));
        }else if(address>=0x100000u&&address<0x100020u)byte=f->scroll_ports[address-0x100000u];
        else if(address>=0x140000u&&address<0x140020u)byte=f->control_ports[address-0x140000u];
        else if(address>=0x184000u&&address<0x184002u)byte=(uint8_t)(f->back_color>>((address&1u)?0:8));
        else return fault(f,f->driver.pc,address);
        value=(value<<8)|byte;
    }
    *out=value;return WF_OK;
}
static wf_status_t bus_read(void *user,uint32_t at,uint32_t *out,unsigned width)
{
    wf_jo_frontend_t *f=user;
    if(!out||!width||width>4u)return WF_BAD_STATE;
    at&=0xffffffu;
    /* A native READ16 samples one complete cabinet word, not two repeated
     * physical polls just to split that word into its bytes. Keep bytewise
     * decoding outside this common path's register/stack frame. */
    if(width==2u&&at>=0x140020u&&at<0x140028u&&!(at&1u)){
        *out=input_word(f,(at-0x140020u)/2u);return WF_OK;
    }
    return bus_read_bytes(f,at,out,width);
}
static __attribute__((noinline)) wf_status_t bus_store_audio(wf_jo_frontend_t *f,uint32_t value)
{
    if(wf_r18_scsp_audio(f->config.runtime,(uint16_t)value,NULL)!=WF_OK)
        return fault(f,f->driver.pc,0x14000cu);
    /* Complete both original services before changing the control port.
     * Isolate their live values from the frequent IRQ acknowledgement path. */
    if(!wf_jo_audio_service_cooperate())return fault(f,f->driver.pc,0x14000cu);
    write16(f->control_ports+0xcu,(uint16_t)value);return WF_OK;
}
static wf_status_t bus_store(void *user,uint32_t at,uint32_t value,unsigned width)
{
    wf_jo_frontend_t *f=user;
    if(!width||width>4u)return WF_BAD_STATE;
    at&=0xffffffu;
    if(at==0x14000cu&&width==2u)return bus_store_audio(f,value);
    uint8_t *target=NULL;
    if(at>=0x100000u&&at+width<=0x100020u)target=f->scroll_ports+at-0x100000u;
    else if(at>=0x140000u&&at+width<=0x140020u)target=f->control_ports+at-0x140000u;
    else if(at==0x184000u&&width==2u){f->back_color=(uint16_t)value;return WF_OK;}
    if(!target)return fault(f,f->driver.pc,at);
    /* The frequent original IRQ acknowledgements are word writes. Constant
     * shifts avoid a libgcc variable-shift call for each byte on SH-2. */
    if(width==2u)write16(target,(uint16_t)value);
    else if(width==4u){write16(target,(uint16_t)(value>>16));write16(target+2,(uint16_t)value);}
    else if(width==1u)target[0]=(uint8_t)value;
    else{target[0]=(uint8_t)(value>>16);target[1]=(uint8_t)(value>>8);target[2]=(uint8_t)value;}
    return WF_OK;
}

static wf_status_t stage_records(void *user,const wf_r18_sprite_record_t *records,size_t count)
{
    wf_jo_frontend_t *f=user;size_t capacity;
    wf_r18_sprite_record_t *workspace=wf_r18_present_records_buffer(&capacity);
    if(count>capacity||(!records&&count))return WF_BAD_STATE;
    if(count)memmove(workspace,records,count*sizeof *records);
    f->pending_records=count;f->pending=1;
    return WF_OK;
}
wf_status_t wf_jo_frontend_flush(wf_jo_frontend_t *f)
{
    if(!f||!f->driver.ram)return WF_BAD_STATE;
    /* A previous application owner may already have queued this Jo tag.
     * Leave borrowed records staged until __jo_core_synch opens the next. */
    if(wf_jo_video_frame_queued())return f->pending?WF_BLOCKED:WF_OK;
    if(f->pending){
        wf_status_t status=wf_r18_present_records_commit(f->config.video,f->pending_records);
        if(status!=WF_OK)return fault(f,f->driver.pc,0x293eu);
        f->pending=0;
    }
    wf_status_t status=wf_jo_video_commit();
    if(status!=WF_OK)return fault(f,f->driver.pc,0x56445032u);
    ++f->frame_commits;return WF_OK;
}
static wf_status_t synchronise_frame(wf_jo_frontend_t *f,wf_r18_frontend_driver_t *driver)
{
    if(!wf_jo_video_frame_queued()&&wf_jo_frontend_flush(f)!=WF_OK)return WF_BLOCKED;
    __jo_core_synch();
    if(!wf_r18_scsp_service())return fault(f,driver->pc,0x2052u);
    if(!wf_jo_clock_advance(f->config.clock,jo_core_get_vblank_count()))return WF_BAD_STATE;
    observe(f);return WF_OK;
}
static unsigned prepare_hint(const wf_jo_frontend_t *f,const wf_r18_frontend_driver_t *driver)
{
    /* These original waits have enough room to prepare the next raw world.
     * This is only a prediction: bind_world checks the actual requested name
     * and falls back to its normal read if input redirects the original flow. */
    if(driver->phase==WF_R18_DRIVER_BOOT&&driver->scene.boot.pc==0x1e7cu)
        return 4u;
    const wf_r18_match_driver_t *match=&f->match;
    if(driver->phase==WF_R18_DRIVER_AC0&&match->frontend==driver&&
       match->generation==driver->generation&&match->flow.pending&&
       match->flow.request.kind==WF_R18_MATCH_CALL&&match->flow.request.owner==0x7b70u&&
       match->flow.request.call_pc==0xc02u&&f->bound_world==4u&&
       (driver->pc==0x7cdeu||driver->pc==0x7dfcu||driver->pc==0x7e5eu)){
        static const uint8_t worlds[10]={0,5,1,5,0,5,0,1,5,0};
        unsigned stage=read16(driver->ram+0x162u)&255u;
        if(stage<10u)return worlds[stage];
    }
    return 7u;
}
static wf_status_t prepare_slice(wf_jo_frontend_t *f,unsigned hint)
{
    if(hint>=7u||hint==f->bound_world)return WF_OK;
    if(!wf_jo_scene_prepare_active(world_names[hint])){
        int status=wf_jo_scene_prepare_begin(world_names[hint]);
        /* A smaller valid memory lease can use the original synchronous
         * loader. The optimization never claims a different memory owner. */
        if(status==WF_JO_ASSET_CAPACITY)return WF_OK;
        if(status)return fault(f,f->driver.pc,0x50524500u|hint);
    }
    if(wf_jo_scene_prepare_step()<0)return fault(f,f->driver.pc,0x50524500u|hint);
    if(!wf_jo_audio_service_cooperate())return fault(f,f->driver.pc,0x2052u);
    return WF_OK;
}
static wf_status_t next_tick(wf_jo_frontend_t *f,wf_r18_frontend_driver_t *driver)
{
    observe(f);
    if(!wf_r18_scsp_service())return fault(f,driver->pc,0x2052u);
    if(!wf_jo_clock_advance(f->config.clock,jo_core_get_vblank_count()))return WF_BAD_STATE;
    if(f->pending||wf_jo_video_pending_changes()){
        /* Debt is elapsed arcade time, not permission to discard an image.
         * Finish any older frozen tag first, then publish this complete dirty
         * state before another IRQ3 can replace its records or live palette.
         * Unchanged static states can still consume elapsed ticks directly. */
        wf_status_t status;
        if(wf_jo_video_frame_queued()){
            status=synchronise_frame(f,driver);if(status!=WF_OK)return status;
        }
        if(wf_jo_frontend_flush(f)!=WF_OK)return WF_BLOCKED;
        status=synchronise_frame(f,driver);if(status!=WF_OK)return status;
    }
    while(!wf_jo_clock_take(f->config.clock)){
        wf_status_t status=synchronise_frame(f,driver);if(status!=WF_OK)return status;
    }
    f->high=1;++f->arcade_ticks;
    uint32_t generation=driver->generation;
    unsigned hint=prepare_hint(f,driver);
    wf_status_t result=WF_OK;
    for(unsigned player=0;player<4u;player++)sampled_controls[player]=controls(player);
    sampled_controls_owner=f;
    for(unsigned i=0;i<17u;i++)if((driver->control.sr&0x0700u)<0x0200u){
        if(wf_r18_frontend_driver_on_raster(driver)!=WF_OK){result=WF_BLOCKED;goto irq_complete;}
        ++f->raster_irqs;
        if(driver->generation!=generation)goto irq_complete;
    }
    if((driver->control.sr&0x0700u)<0x0300u){
        /* on_vblank already invokes platform.commit after the IRQ. */
        if(wf_r18_frontend_driver_on_vblank(driver)!=WF_OK){result=WF_BLOCKED;goto irq_complete;}
        ++f->vblank_irqs;
    }
irq_complete:
    sampled_controls_owner=NULL;
    /* START can replace the original owner during IRQ3. Do not advance its
     * captured hint after that diversion. Already prepared data is only an
     * immutable file cache, so it remains valid across that control change;
     * the actual requested scene name still decides whether it is used. */
    if(result==WF_OK&&driver->generation==generation)result=prepare_slice(f,hint);
    observe(f);return result;
}
static wf_status_t wait_level(void *user,wf_r18_frontend_driver_t *driver,unsigned high)
{
    wf_jo_frontend_t *f=user;
    /* The arcade input bit belongs to its 57.44 Hz field, not to whichever
     * portion of the 59.826 Hz Saturn field the SH-2 happens to be in. */
    if(high&&!f->high)return next_tick(f,driver);
    f->high=(uint8_t)!!high;return WF_OK;
}
static wf_status_t wait_vblank(void *user,wf_r18_frontend_driver_t *driver)
{wf_jo_frontend_t *f=user;f->high=0;return next_tick(f,driver);}

static wf_status_t scene_clear(void *user,uint32_t owner,unsigned complete)
{
    wf_jo_frontend_t *f=user;(void)owner;
    if(complete){if(f->scene_gate)f->scene_gate=2;return WF_OK;}
    wf_status_t drained=wf_jo_video_drain_overlap(NULL,NULL);
    if(drained!=WF_OK)return drained;
    wf_jo_video_gate();wf_jo_video_layers(0,0,0);f->scene_gate=1;
    /* If another list is completed before the next publication it replaces
     * this empty staged list. In either case Jo erases the old framebuffer
     * and the gate releases only on the matching new sprite/map snapshot. */
    return stage_records(f,NULL,0);
}
static wf_status_t bind_world(void *user,unsigned mode)
{
    wf_jo_frontend_t *f=user;
    if(f->bound_world==mode){if(f->scene_gate)f->scene_gate=2;return WF_OK;}
    if(mode>=7u||!wf_jo_assets_scenes_ready())return fault(f,f->driver.pc,0x4d415000u|mode);
    wf_status_t status=scene_clear(f,0,0);if(status!=WF_OK)return status;
    uint32_t bytes;
    if(wf_jo_assets_file_size(world_names[mode],&bytes)||bytes>f->config.scene_capacity)
        return fault(f,f->driver.pc,0x4d415000u|mode);
    const void *prepared=NULL;uint32_t prepared_bytes=0;
    int ready=wf_jo_scene_prepare_view(world_names[mode],&prepared,&prepared_bytes);
    if(ready<0)return fault(f,f->driver.pc,0x4d415000u|mode);
    if(ready){
        if(prepared_bytes!=bytes)return fault(f,f->driver.pc,0x4d415000u|mode);
#ifdef WF_JO_FRONTEND_TEST
        memcpy(f->config.scene_blob,prepared,bytes);
#else
        jo_memcpy(f->config.scene_blob,prepared,bytes);
#endif
        wf_jo_scene_prepare_cancel();
    }else{
        wf_jo_scene_prepare_cancel();
        if(wf_jo_assets_file_read(world_names[mode],0,f->config.scene_blob,bytes))
            return fault(f,f->driver.pc,0x4d415000u|mode);
    }
    if(read16((const uint8_t *)f->config.scene_blob+6)!=mode||
       wf_r18_ring_bind(f->config.scene_blob,bytes))return fault(f,f->driver.pc,0x4d415000u|mode);
    wf_r18_video_ring_bind(f->config.video);
    f->config.video->palette_committed[WF_R18_PALETTE_FG]=0;
    f->config.video->palette_committed[WF_R18_PALETTE_BG]=0;
    f->bound_world=(uint16_t)mode;f->scene_gate=2;
    return WF_OK;
}
static wf_status_t commit(void *user,wf_r18_frontend_driver_t *driver)
{
    wf_jo_frontend_t *f=user;wf_r18_video_t *video=f->config.video;
    if(wf_r18_hud_commit(video)!=WF_OK)return WF_BLOCKED;
    for(unsigned role=WF_R18_PALETTE_TEXT;role<=WF_R18_PALETTE_BG;role++)
        if(wf_r18_palettes_commit(video,(wf_r18_palette_role_t)role)!=WF_OK)return WF_BLOCKED;
    unsigned layers=4u,opaque=4u,sprites=3u;
    if(driver->world_visible){
        uint8_t priority=(uint8_t)read16(f->control_ports+0x10);
        if(wf_r18_ring_update_deferred((int16_t)read16(f->scroll_ports),
               (int16_t)(2048u-read16(f->scroll_ports+2)),
               (int16_t)read16(f->scroll_ports+4),
               (int16_t)(2048u-read16(f->scroll_ports+6)),priority))return WF_BLOCKED;
        layers=7u;opaque=priority==0x78u?2u:1u;sprites=priority==0x7cu?2u:3u;
    }
    if(f->scene_gate==1u)layers=opaque=sprites=0;
    else if(f->scene_gate==2u)f->scene_gate=0;
    wf_jo_video_layers(layers,opaque,sprites);
    return WF_OK;
}
static wf_status_t reset_ready(void *user,uint8_t *ram,size_t bytes)
{
    wf_jo_frontend_t *f=user;
    return f->config.reset_ready?f->config.reset_ready(f->config.user,ram,bytes):WF_OK;
}
static wf_status_t support_rom(void *user,uint32_t at,uint8_t *out,size_t bytes)
{return rom_read(user,at,out,bytes);}
static wf_status_t support_palette(void *user,uint32_t at,uint16_t value)
{
    wf_jo_frontend_t *f=user;wf_r17_fg0_write_t write={at,value,2};
    return wf_r18_video_batch(f->config.runtime,&write,1,f->config.video);
}
static wf_status_t extension(void *user,uint32_t owner,wf_r18_prematch_t *machine)
{
    wf_jo_frontend_t *f=user;
    if(owner==0x285dau){
        wf_r18_frontend_world_t world={.ram=machine->ram,.ram_bytes=WF_JO_FRONTEND_RAM_BYTES,
            .regs=machine->regs,.a=machine->a,.read_rom=rom_read,.user=f};
        wf_status_t status=wf_r18_frontend_world_call(&world,owner);
        if(status!=WF_OK)machine->dependency=world.blocked_dependency;
        return status;
    }
    wf_r18_match_support_t support={.machine=machine,.fg_0c0000=f->config.video->fg0,
        .fg_bytes=sizeof f->config.video->fg0,.read_rom=support_rom,
        .write_palette=support_palette,.user=f};
    wf_status_t status=wf_r18_match_support_call(&support,owner);
    memset(f->config.video->hud_dirty,0xff,sizeof f->config.video->hud_dirty);
    return status;
}

wf_status_t wf_jo_frontend_init(wf_jo_frontend_t *f,const wf_jo_frontend_config_t *config,uint32_t seed)
{
    if(!f||!config||!config->runtime||!config->video||!config->world||!config->display||
       !config->clock||!config->ram||config->ram_bytes<WF_JO_FRONTEND_RAM_BYTES||
       !config->scene_blob||config->scene_capacity<131072u||!config->priority_140010||
       !config->render_counts||!config->combat_read_ram||!wf_jo_assets_scenes_ready()||
       !wf_jo_audio_diagnostic.initialized)return WF_BAD_STATE;
    if(overlaps(config->ram,WF_JO_FRONTEND_RAM_BYTES,f,sizeof *f)||
       overlaps(config->ram,WF_JO_FRONTEND_RAM_BYTES,config->runtime,sizeof *config->runtime)||
       overlaps(config->ram,WF_JO_FRONTEND_RAM_BYTES,config->video,sizeof *config->video)||
       overlaps(config->ram,WF_JO_FRONTEND_RAM_BYTES,config->world,sizeof *config->world)||
       overlaps(config->ram,WF_JO_FRONTEND_RAM_BYTES,config->scene_blob,config->scene_capacity))return WF_BAD_STATE;
    wf_jo_frontend_config_t borrowed=*config;
    wf_jo_scene_prepare_cancel();
    memset(f,0,sizeof *f);f->config=borrowed;config=&f->config;f->bound_world=0xffffu;
    memset(config->ram,0,0x60u);
    write16(config->ram+0x5c,(uint16_t)(seed>>16));write16(config->ram+0x5e,(uint16_t)seed);
    wf_r18_title_portrait_divisor_set(1u);wf_r18_prematch_speech_divisor_set(1u);
    /* Initial loading precedes the first arcade tick. Prepare the first
     * menu here, leaving its original owner to bind/publish it at tick4.
     * The already primed cue02 remains silent until its original command. */
    int preparing=wf_jo_scene_prepare_begin(world_names[3]);
    if(preparing!=WF_JO_ASSET_CAPACITY){
        if(preparing)return WF_BAD_STATE;
        do{
            preparing=wf_jo_scene_prepare_step();
            if(preparing<0||!wf_jo_audio_service_cooperate()){
                wf_jo_scene_prepare_cancel();return WF_BAD_STATE;
            }
        }while(!preparing);
    }
    wf_jo_clock_start(config->clock,jo_core_get_vblank_count());
    wf_r18_frontend_driver_config_t driver_config={.ram=config->ram,.ram_bytes=WF_JO_FRONTEND_RAM_BYTES,
        .runtime=config->runtime,.video=config->video,.ssp=0x1c3bd4u,.usp=0x1c3fd4u,.sr=0x2700u,
        .platform={.read_rom=rom_read,.read_bus=bus_read,.store_bus=bus_store,
        .wait_level=wait_level,.wait_vblank=wait_vblank,.commit=commit,.bind_world=bind_world,
        .submit_records=stage_records,.user=f,.reset_ready=reset_ready,.scene_clear=scene_clear}};
    wf_status_t status=wf_r18_frontend_driver_init(&f->driver,&driver_config);
    if(status!=WF_OK)wf_jo_scene_prepare_cancel();
    return status;
}
wf_status_t wf_jo_frontend_run(wf_jo_frontend_t *f)
{
    if(!f||!f->driver.ram)return WF_BAD_STATE;
    wf_r18_frontend_driver_t *driver=&f->driver;
    for(;;){
        if(driver->phase!=WF_R18_DRIVER_AC0&&wf_r18_frontend_driver_run(driver)!=WF_OK)
            return fault(f,driver->pc,driver->blocked_dependency);
        uint32_t generation=driver->generation;
        if(wf_r18_match_driver_init(&f->match,driver,extension,f)!=WF_OK)return WF_BAD_STATE;
        wf_status_t status=wf_r18_match_driver_run(&f->match);
        if(generation!=driver->generation&&driver->phase==WF_R18_DRIVER_MODE_PLATE)continue;
        if(status!=WF_OK)return fault(f,f->match.blocked_pc,f->match.blocked_dependency);
        break;
    }
    wf_runtime_t *runtime=f->config.runtime;
    if(wf_r18_runtime_handoff_f8c(runtime,driver->ram,driver->ram_bytes,&driver->regs)!=WF_OK)
        return fault(f,0xf8cu,0xf8cu);
    if(f->bound_world!=runtime->g.global_007e||
       read16((const uint8_t *)f->config.scene_blob+6)!=runtime->g.global_007e)
        return fault(f,0xf8cu,0x1c007eu);
    for(unsigned side=0;side<2u;side++)for(unsigned i=0;i<16u;i++){
        const uint8_t *p=driver->ram+0x16d6u+side*128u+i*8u;
        f->config.world->cache[side][i]=(wf_r18_world_transport_t){read32(p),read32(p+4)};
    }
    f->config.world->animation_count[0]=f->config.world->animation_count[1]=0;
    *f->config.priority_140010=(uint8_t)read16(f->control_ports+0x10);
    f->config.video->read_rom=wf_r18_rom_read;
    f->config.video->read_ram=f->config.combat_read_ram;
    f->config.video->read_user=f->config.combat_read_user;
    if(wf_r18_display_init(f->config.display,runtime,wf_r18_rom_read,NULL)!=WF_OK)
        return fault(f,0xf8cu,0x27048u);
    wf_jo_clock_start(&f->combat_coin_clock,jo_core_get_vblank_count());
    f->combat_coin_clock.remainder=f->config.clock->remainder;
    wf_jo_scene_prepare_cancel();
    f->handed_off=1;return WF_OK;
}

static wf_status_t scene_ram(wf_runtime_t *runtime,uint32_t at,uint16_t *out,void *user)
{
    wf_jo_frontend_t *f=user;(void)runtime;at&=0xffffffu;
    if(!out||at<0x1c0000u||at>0x1c3ffeu)return WF_BLOCKED;
    *out=read16(f->driver.ram+at-0x1c0000u);return WF_OK;
}
wf_status_t wf_jo_frontend_combat_coins(wf_jo_frontend_t *f)
{
    if(!f||!f->handed_off||!f->driver.ram)return WF_BAD_STATE;
#if defined(JO_COMPILE_WITH_PROFILER_SUPPORT) && !defined(WF_JO_FRONTEND_TEST)
    unsigned began_frt=jo_profile_ticks(),began_ms=jo_get_ticks(),began_irqs=f->combat_coin_irqs;
#endif
    wf_status_t result=WF_OK;
    wf_runtime_t *runtime=f->config.runtime;uint8_t *raw=f->driver.ram;
    raw[0x4b]=runtime->g.credit_cycle_units_004b;
    raw[0x4e]=runtime->g.credit_remainder_004e;raw[0x4f]=runtime->g.credit_count_004f;
    if(!wf_jo_clock_advance(&f->combat_coin_clock,jo_core_get_vblank_count())){
        result=WF_BAD_STATE;goto complete;
    }
    if(f->combat_coin_clock.pending){
        for(unsigned player=0;player<4u;player++)sampled_controls[player]=controls(player);
        sampled_controls_owner=f;
#if defined(JO_COMPILE_WITH_PROFILER_SUPPORT) && !defined(WF_JO_FRONTEND_TEST)
        ++wf_jo_coin_profile[6];
#endif
    }
    while(wf_jo_clock_take(&f->combat_coin_clock)){
        for(unsigned i=0;i<17u;i++)if((f->driver.control.sr&0x0700u)<0x0200u){
            wf_status_t status=wf_r18_frontend_driver_on_raster(&f->driver);
            runtime->g.credit_remainder_004e=raw[0x4e];runtime->g.credit_count_004f=raw[0x4f];
            if(status!=WF_OK){result=fault(f,f->driver.pc,f->driver.blocked_dependency);goto complete;}
            ++f->combat_coin_irqs;
        }
        /* A terminal wait can leave many complete IRQ2 groups to replay.
         * Keep dynamic PCM filled between groups, retaining this call's
         * input snapshot and its already-admitted clock debt. */
        if(f->combat_coin_clock.pending&&!wf_jo_audio_service_cooperate()){
            result=fault(f,f->driver.pc,0x2052u);goto complete;
        }
    }
complete:
    sampled_controls_owner=NULL;
#if defined(JO_COMPILE_WITH_PROFILER_SUPPORT) && !defined(WF_JO_FRONTEND_TEST)
    {
        unsigned elapsed=(jo_profile_ticks()-began_frt)&0xffffu,ms=jo_get_ticks()-began_ms;
        ++wf_jo_coin_profile[0];wf_jo_coin_profile[1]+=f->combat_coin_irqs-began_irqs;
        wf_jo_coin_profile[2]+=elapsed;if(elapsed>wf_jo_coin_profile[3])wf_jo_coin_profile[3]=elapsed;
        wf_jo_coin_profile[4]+=ms;if(ms>wf_jo_coin_profile[5])wf_jo_coin_profile[5]=ms;
        wf_jo_coin_profile[7]=elapsed;
    }
#endif
    return result;
}
static wf_status_t start_drain_service(void *user)
{
    wf_jo_frontend_t *f=user;
    /* Accepted $978 is still inside IRQ3: no IRQ2/coin callback may enter
     * before $8AE restores the owner's interrupt context. */
    return wf_jo_audio_service_cooperate()?WF_OK:fault(f,0x978u,0x2052u);
}
wf_status_t wf_jo_frontend_combat_start(wf_jo_frontend_t *f,unsigned *accepted)
{
    if(!f||!accepted||!f->handed_off||!f->driver.ram)return WF_BAD_STATE;
    *accepted=0;wf_runtime_t *runtime=f->config.runtime;
    /* $83E/$89E gates. A paid match's participant owners already handle its
     * own START/payment paths; they must not be diverted to mode selection. */
    if(!(runtime->g.gameplay_vblank_enable&0x80u)||(runtime->g.credited_route&0x8000u)||
       !runtime->g.credit_count_004f)return WF_OK;
    uint16_t inputs[4];
    for(unsigned i=0;i<4u;i++)inputs[i]=input_word(f,i);
    unsigned candidate=0;
    if((runtime->g.config_0066&0x0800u)==0x0800u)
        candidate=!(inputs[0]&0x80u)||(!(inputs[1]&0x80u)&&runtime->g.credit_count_004f>=2u);
    else for(unsigned i=0;i<4u;i++)candidate|=!(inputs[i]&0x80u);
    if(!candidate)return WF_OK;
    size_t capacity;wf_r18_sprite_record_t *records=wf_r18_present_records_buffer(&capacity);
    size_t count=wf_r18_present_records_count();if(count>capacity)return WF_BAD_STATE;
    wf_status_t status=wf_r18_scene_resume(runtime,f->driver.ram,f->driver.ram_bytes,
        f->driver.ram,f->driver.ram_bytes,f->config.video,f->config.world,records,count,f->config.render_counts);
    if(status!=WF_OK)return status;
    wf_r17_m68k_shadow_t previous_regs=f->driver.regs;uint32_t previous_a[7];
    memcpy(previous_a,f->driver.a,sizeof previous_a);f->driver.regs=runtime->m68k_regs;
    wf_r18_frontend_boot_context_t context={f->driver.ram,f->driver.ram_bytes,&f->driver.regs,f->driver.a,NULL,NULL};
    int carry=wf_r18_frontend_start_0978(&context,inputs,NULL);
    runtime->g.credit_remainder_004e=f->driver.ram[0x4e];runtime->g.credit_count_004f=f->driver.ram[0x4f];
    if(carry<0)return fault(f,0x978u,0x978u);
    if(!carry){f->driver.regs=previous_regs;memcpy(f->driver.a,previous_a,sizeof previous_a);return WF_OK;}
    uint32_t drain_fields=0;
    status=wf_jo_video_drain_overlap_fields(start_drain_service,f,&drain_fields);
    if(status!=WF_OK)return status;
    /* $978 already performed its original clear. Never export the old
     * actors a second time after this point. The original $8AE owns all
     * remaining stack, SR, display-clear and generation effects. */
    for(unsigned i=0;i<4u;i++)write16(f->scroll_ports+i*2u,f->config.display->scroll[i]);
    write16(f->control_ports+0x10,*f->config.priority_140010);
    f->bound_world=runtime->g.global_007e;f->driver.world_visible=1;
    f->config.video->read_rom=wf_r18_rom_read;f->config.video->read_ram=scene_ram;f->config.video->read_user=f;
    /* The original handoff excludes scene export/$978 transport. Charge only
     * the new display closure, then reanchor once at the original point. A
     * field arriving after this sample belongs to the next owner. */
    uint32_t anchor=jo_core_get_vblank_count();
    wf_jo_clock_t anchored=*f->config.clock;
    anchored.last_field=anchor-drain_fields;
    if(!wf_jo_clock_advance(&anchored,anchor))return WF_BAD_STATE;
    f->combat_start_debt=anchored.pending?anchored.pending-1u:0u;
    anchored.pending=0;*f->config.clock=anchored;
    f->pending=0;f->high=1;f->handed_off=0;
    f->driver.control.sr=(uint16_t)((f->driver.control.sr&0x001fu)|0x2301u);
    status=wf_r18_frontend_driver_start_8ae(&f->driver);
    if(status!=WF_OK)return fault(f,f->driver.pc,f->driver.blocked_dependency);
    *accepted=1;observe(f);return WF_OK;
}
static wf_status_t post_owner(wf_jo_frontend_t *f,uint32_t owner)
{
    if(owner==0x206feu)return wf_r18_referee_clear_206fe(f->config.runtime,f->config.video);
    if(owner==0x2067cu)return wf_r18_referee_number_2067c(f->config.runtime,
        (uint16_t)f->driver.regs.d[0],f->config.video);
    if(owner!=0x11b6u&&owner!=0x122cu&&owner!=0x1afcu&&owner!=0x1b1eu&&owner!=0x1b4eu&&owner!=0xb608u&&
       owner!=0xbda6u&&owner!=0xae20u&&owner!=0x15f4u&&owner!=0x139cu&&owner!=0x160eu){
        wf_status_t status=wf_r18_match_driver_owner(&f->match,owner);
        if(status==WF_OK&&owner==0x65c0u&&f->config.records_ready)
            return f->config.records_ready(f->config.user,f->driver.ram,f->driver.ram_bytes);
        return status;
    }
    wf_r18_post_tag_t cursor;wf_r18_post_tag_init(&cursor,owner);
    wf_r18_post_tag_context_t context={&f->match.machine,rom_read,f};
    uint32_t generation=f->driver.generation;
    for(;;){
        f->driver.pc=cursor.pc;int result=wf_r18_post_tag_step(&cursor,&context);
        if(result==WF_R18_POST_AC0||result==WF_R18_POST_RETURN)return WF_OK;
        if(result==WF_R18_POST_BOUNDARY&&cursor.pc==0x6fcu){
            memcpy(f->driver.a,context.machine->a,sizeof f->driver.a);
            return wf_r18_frontend_driver_resume_boot(&f->driver,0x6fcu);
        }
        if(result==WF_R18_POST_MORE)continue;
        if(result!=WF_R18_POST_REQUEST)break;
        wf_status_t status=WF_BLOCKED;uint16_t value=0;
        if(cursor.request.kind==WF_R18_POST_CALL)status=post_owner(f,cursor.request.owner);
        else if(cursor.request.kind==WF_R18_POST_WRITE16){
            uint32_t at=cursor.request.address;
            if((at>=0x180000u&&at<0x184000u)||(at>=0x186000u&&at<0x186800u)||
               (at>=0x188000u&&at<0x188800u)){
                wf_r17_fg0_write_t write={at,cursor.request.value,2};
                status=wf_r18_video_batch(f->config.runtime,&write,1,f->config.video);
            }else status=bus_store(f,at,cursor.request.value,2);
        }else if(cursor.request.kind==WF_R18_POST_READ16||cursor.request.kind==WF_R18_POST_READ8){
            uint32_t word;status=bus_read(f,cursor.request.address,&word,
                cursor.request.kind==WF_R18_POST_READ8?1u:2u);
            if(status==WF_OK)value=(uint16_t)word;
        }else if(cursor.request.kind==WF_R18_POST_SYNC||cursor.request.kind==WF_R18_POST_LEVEL){
            memcpy(f->driver.a,context.machine->a,sizeof f->driver.a);
            status=cursor.request.kind==WF_R18_POST_LEVEL?
                wait_level(f,&f->driver,cursor.request.value):wait_vblank(f,&f->driver);
            memcpy(context.machine->a,f->driver.a,sizeof f->driver.a);
        }
        if(generation!=f->driver.generation)return WF_BLOCKED;
        if(wf_r18_post_tag_complete(&cursor,&context,status,value)<0)break;
    }
    return fault(f,cursor.pc,cursor.blocked_dependency);
}
wf_status_t wf_jo_frontend_after_combat(wf_jo_frontend_t *f,uint32_t owner)
{return wf_jo_frontend_after_combat_at_level(f,owner,0u);}
wf_status_t wf_jo_frontend_after_combat_at_level(wf_jo_frontend_t *f,uint32_t owner,unsigned high)
{
    if(!f||!f->handed_off||high>1u)return WF_BAD_STATE;
    if(owner!=0x11b6u&&owner!=0x122cu&&owner!=0x1afcu&&owner!=0x1b1eu&&owner!=0x1b4eu&&owner!=0x1120u)return WF_BAD_STATE;
    size_t capacity;wf_r18_sprite_record_t *records=wf_r18_present_records_buffer(&capacity);
    size_t count=wf_r18_present_records_count();if(count>capacity)return WF_BAD_STATE;
    wf_status_t status=wf_r18_scene_resume(f->config.runtime,f->driver.ram,f->driver.ram_bytes,
        f->driver.ram,f->driver.ram_bytes,f->config.video,f->config.world,records,count,f->config.render_counts);
    if(status!=WF_OK)return status;
    for(unsigned i=0;i<4u;i++)write16(f->scroll_ports+i*2u,f->config.display->scroll[i]);
    write16(f->control_ports+0x10,*f->config.priority_140010);
    f->bound_world=f->config.runtime->g.global_007e;f->driver.world_visible=1;
    f->driver.regs=f->config.runtime->m68k_regs;
    f->config.video->read_rom=wf_r18_rom_read;f->config.video->read_ram=scene_ram;
    f->config.video->read_user=f;
    /* Combat consumed its own clock. Preserve the rational fractional phase
     * but never replay its elapsed IRQs into the retained cabinet controller. */
    f->config.clock->last_field=jo_core_get_vblank_count();f->config.clock->pending=0;
    /* This is the caller's original arcade level at the ACK seam. The
     * elapsed physical time in scene_resume is transport cost, not a LOW
     * event. $2A06 and the following native owners decide their own waits. */
    f->pending=0;f->high=(uint8_t)high;f->handed_off=0;
    uint32_t generation=f->driver.generation;
    if(owner==0x1120u){
        status=wf_r18_match_driver_owner(&f->match,0x8f50u);
        for(unsigned i=0;status==WF_OK&&i<33u&&generation==f->driver.generation;i++){
            memcpy(f->driver.a,f->match.machine.a,sizeof f->driver.a);
            status=wait_vblank(f,&f->driver);
            memcpy(f->match.machine.a,f->driver.a,sizeof f->driver.a);
        }
        if(status==WF_OK&&generation==f->driver.generation){
            uint16_t sequence=(uint16_t)(read16(f->driver.ram+0x166u)+1u);
            write16(f->driver.ram+0x166u,sequence);
            memcpy(f->driver.a,f->match.machine.a,sizeof f->driver.a);
            if(sequence>=14u)status=wf_r18_frontend_driver_resume_boot(&f->driver,0x6fcu);
            else{f->driver.phase=WF_R18_DRIVER_AC0;f->driver.pc=0xac0u;}
        }
    }else status=post_owner(f,owner);
    unsigned diverted=generation!=f->driver.generation&&f->driver.phase==WF_R18_DRIVER_MODE_PLATE;
    if(status!=WF_OK&&!diverted)return fault(f,f->driver.pc,f->match.blocked_dependency);
    if(!diverted&&f->driver.phase!=WF_R18_DRIVER_BOOT){
        memcpy(f->driver.a,f->match.machine.a,sizeof f->driver.a);
        f->driver.phase=WF_R18_DRIVER_AC0;f->driver.pc=0xac0u;
    }
    return WF_OK;
}
