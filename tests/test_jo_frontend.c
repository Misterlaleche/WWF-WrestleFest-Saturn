/* Run the actual Jo frontend transport and native scene owners together.
 * Only hardware time/input/SCSP/VDP publication are deterministic host ports. */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "native/assets_fixture.h"
#include "wf_gameplay_bindings.h"
#include "wf_jo_audio.h"
#include "wf_jo_frontend.h"
#include "wf_jo_scene_prepare.h"
#include "wf_jo_video.h"
#include "wf_r18_hud.h"
#include "wf_r18_present.h"
#include "wp1980_input.h"
#include "wf_payment.h"
#include "wf_jo_frontend_test_platform.h"

static wf_jo_frontend_t front;
static wf_runtime_t runtime;
static wf_gameplay_binding_context_t gameplay;
static wf_phase_binding_t bindings[48];
static wf_r18_video_t video;
static wf_r18_world_ctx_t world;
static wf_r18_display_t display;
static wf_jo_clock_t clock_state;
static uint8_t raw[WF_JO_FRONTEND_RAM_BYTES],scene[131072],priority;
static unsigned fields,field_stride=1,queued,commits,gate_count,sound_count;
static unsigned layers_state,opaque_state,sprites_state;
static unsigned active_inputs,wanted_royal;
static unsigned serviced_sound_count;
static unsigned require_dirty_frames,dirty_state,published_state;
static wf_status_t (*native_commit)(void *,wf_r18_frontend_driver_t *);
static unsigned input_port_queries,input_key_queries;
static uint8_t pads[4];
static uint16_t sound_log[256];
static unsigned coin_audio_watch,coin_audio_calls,coin_audio_last_irqs,coin_audio_max_gap,coin_audio_fail_at;
static unsigned store_audio_watch,store_audio_order,store_audio_fail;
static unsigned coin_input_count;
static uint32_t coin_inputs[66u*17u*4u];
volatile wf_jo_audio_diagnostic_t wf_jo_audio_diagnostic={.initialized=1};

unsigned jo_core_get_vblank_count(void){return fields;}
int jo_get_player_input_port(int player){++input_port_queries;return player>=0&&player<4?player:-1;}
bool jo_is_input_key_pressed(unsigned port,unsigned key){++input_key_queries;return (pads[port]&key)!=0;}
void __jo_core_synch(void){assert(queued);queued=0;fields+=field_stride;wf_host_assets_frame(fields);}
bool wf_r18_scsp_service(void){return true;}
bool wf_jo_audio_service_cooperate(void){
    if(store_audio_watch){
        assert(store_audio_order==1u&&front.control_ports[12]==0x56u&&front.control_ports[13]==0x78u);
        store_audio_order=2u;front.control_ports[12]=0x9au;front.control_ports[13]=0xbcu;
        if(store_audio_fail==2u)return false;
    }
    if(coin_audio_watch){
        assert(!front.driver.coin_active);
        unsigned gap=front.combat_coin_irqs-coin_audio_last_irqs;
        assert(gap==17u);
        if(gap>coin_audio_max_gap)coin_audio_max_gap=gap;
        coin_audio_last_irqs=front.combat_coin_irqs;++coin_audio_calls;
        /* Physical input can change during service, but this recovery owns
         * one snapshot for every original IRQ input request. */
        pads[0]=JO_KEY_C|JO_KEY_UP;
        if(coin_audio_calls==coin_audio_fail_at)return false;
    }
    serviced_sound_count=sound_count;return true;
}
wf_status_t wf_r18_scsp_audio(wf_runtime_t *r,uint16_t word,void *user)
{
    (void)r;(void)user;
    if(store_audio_watch){
        assert(store_audio_order==0u&&front.control_ports[12]==0x12u&&front.control_ports[13]==0x34u);
        store_audio_order=1u;front.control_ports[12]=0x56u;front.control_ports[13]=0x78u;
        if(store_audio_fail==1u)return WF_BLOCKED;
    }
    assert((word&0xff00u)==0x3100u);assert(sound_count<256u);sound_log[sound_count++]=word;return WF_OK;
}
int wf_jo_video_frame_queued(void){return (int)queued;}
int wf_jo_video_pending_changes(void){return dirty_state!=published_state;}
wf_status_t wf_jo_video_commit(void)
{assert(!queued);assert(serviced_sound_count==sound_count);queued=1;++commits;published_state=dirty_state;return WF_OK;}
void wf_jo_video_gate(void){++gate_count;}
void wf_jo_video_layers(unsigned layers,unsigned opaque,unsigned sprites)
{assert(!(layers&~7u)&&!(opaque&~layers)&&sprites<=7u);layers_state=layers;opaque_state=opaque;sprites_state=sprites;}
static wf_status_t changed_frame(void *user,wf_r18_frontend_driver_t *driver)
{
    /* Model a complete changing palette/map at every actual native IRQ3.
     * A slow physical field may accrue debt, but cannot replace this state
     * with the next completed state before the transport publishes it. */
    assert(published_state==dirty_state&&"A completed dirty arcade frame was skipped");
    wf_status_t status=native_commit(user,driver);
    if(status==WF_OK)++dirty_state;
    return status;
}

static unsigned records_notifications;
static wf_status_t records_ready(void *user,const uint8_t *ram,size_t bytes)
{
    (void)user;
    assert(ram==front.driver.ram&&bytes==WF_JO_FRONTEND_RAM_BYTES);
    assert(!front.handed_off);
    ++records_notifications;
    return WF_OK;
}
static wf_status_t combat_ram(wf_runtime_t *r,uint32_t at,uint16_t *out,void *u)
{(void)r;(void)at;(void)out;(void)u;return WF_BLOCKED;}
static uint16_t fixture_input(void *user,const wf_jo_frontend_t *f,unsigned port,uint16_t physical)
{
    (void)user;
    if(coin_audio_watch){
        assert(coin_input_count<sizeof coin_inputs/sizeof coin_inputs[0]);
        coin_inputs[coin_input_count++]=((uint32_t)port<<16)|physical;
    }
    if(!active_inputs||f->driver.phase==WF_R18_DRIVER_AC0)return physical;
    if(!wanted_royal&&f->driver.phase==WF_R18_DRIVER_MODE){
        uint16_t word=(uint16_t)(physical|0xffu);if(port)return word;
        if(f->driver.ram[0x161]&1u)word&=(uint16_t)~1u;else word&=(uint16_t)~0x10u;
        return word;
    }
    if(!wanted_royal&&f->driver.phase==WF_R18_DRIVER_ROSTER){
        uint16_t word=(uint16_t)(physical|0xffu);
        if(!port&&(f->arcade_ticks&7u)<4u)word&=(uint16_t)~0x10u;
        return word;
    }
    return wp1980_frontend_input(f->driver.phase,f->driver.ram,f->arcade_ticks,port,physical);
}
static void physical_inputs(void)
{
    uint32_t word;
    /* The transport's constant-width stores must preserve the bus's byte
     * order and truncation for every accepted width, including width3. */
    for(unsigned width=1;width<=4u;width++){
        assert(front.driver.platform.store_bus(&front,0x140010u,0x12345678u,width)==WF_OK);
        assert(front.driver.platform.read_bus(&front,0x140010u,&word,width)==WF_OK);
        uint32_t mask=width==4u?0xffffffffu:((1u<<(width*8u))-1u);
        assert(word==(0x12345678u&mask));
    }
    assert(front.driver.platform.store_bus(&front,0x140010u,0u,4u)==WF_OK);
    pads[0]=JO_KEY_UP|JO_KEY_A|JO_KEY_C;
    assert(front.driver.platform.read_bus(&front,0x140020u,&word,2)==WF_OK);
    assert((word&0x00ffu)==(0xffu&~(4u|16u|64u))&&!(word&0x100u));
    pads[0]=0;pads[1]=JO_KEY_DOWN|JO_KEY_C;
    assert(front.driver.platform.read_bus(&front,0x140022u,&word,2)==WF_OK);
    assert((word&0xffffu)==(0xfcffu&~(8u|64u)));
    assert(front.driver.platform.read_bus(&front,0x140020u,&word,2)==WF_OK&&!(word&0x200u));
    memset(pads,0,sizeof pads);
}
static void coin_period(void)
{
    unsigned irqs=front.combat_coin_irqs;
    unsigned ports=input_port_queries,keys=input_key_queries;
    do{++fields;assert(wf_jo_frontend_combat_coins(&front)==WF_OK);}while(front.combat_coin_irqs==irqs);
    assert(front.combat_coin_irqs==irqs+17u);
    if(input_port_queries-ports>4u||input_key_queries-keys>32u)
        fprintf(stderr,"Repeated physical coin snapshot: ports=%u keys=%u for17 unchanged IRQ2 inputs\n",
            input_port_queries-ports,input_key_queries-keys);
    assert(input_port_queries-ports<=4u&&input_key_queries-keys<=32u);
}
typedef struct input_observation {
    wf_r18_frontend_coin_t coin;
    uint32_t d[3],ram_hash,first;
    uint16_t sr,physical;
} input_observation_t;
static input_observation_t input_observations[51];
static unsigned input_observation_count;
static uint16_t changing_input_first(void *,const wf_jo_frontend_t *,unsigned,uint16_t);
static uint16_t changing_input_next(void *user,const wf_jo_frontend_t *f,unsigned port,uint16_t physical)
{
    (void)user;assert(f==&front&&port==0u&&input_observation_count<51u);
    input_observation_t *o=&input_observations[input_observation_count++];
    o->coin=front.driver.coin;memcpy(o->d,front.driver.regs.d,sizeof o->d);
    o->sr=front.driver.control.sr;o->physical=physical;
    o->first=front.config.input_word==changing_input_first;
    o->ram_hash=2166136261u;
    for(size_t i=0;i<sizeof raw;i++)o->ram_hash=(o->ram_hash^raw[i])*16777619u;
    /* The callback and its return value remain live at every original read.
     * Changing physical pads here must leave this batch's snapshot intact. */
    pads[0]^=JO_KEY_UP|JO_KEY_C;
    front.driver.regs.d[1]=(front.driver.regs.d[1]&~7u)|(input_observation_count&7u);
    front.driver.regs.d[0]^=0x9a5a0000u;
    front.driver.control.sr^=(uint16_t)(input_observation_count&31u);
    raw[0x4e]^=(uint8_t)input_observation_count;
    return (uint16_t)(physical^(0x100u<<(input_observation_count&1u)));
}
static uint16_t changing_input_first(void *user,const wf_jo_frontend_t *f,unsigned port,uint16_t physical)
{
    assert(input_observation_count==0u);
    uint16_t value=changing_input_next(user,f,port,physical);
    front.config.input_word=changing_input_next;return value;
}
static wf_status_t reference_coin_input(void *user,uint32_t at,uint32_t *out,unsigned width)
{
    wf_jo_frontend_t *f=user;
    assert(f==&front&&at==0x140020u&&width==2u&&out);
    /* All four pads were released at the one physical snapshot. Recompute
     * no transport state: supply that known word to the current callback. */
    *out=f->config.input_word(f->config.user,f,0u,0xffffu);return WF_OK;
}
static void input_mutation_test(void)
{
    static wf_jo_frontend_t initial_front,expected_front;
    static wf_runtime_t initial_runtime,expected_runtime;
    static uint8_t initial_raw[sizeof raw],expected_raw[sizeof raw];
    static input_observation_t expected_observations[51];
    uint8_t initial_pads[sizeof pads];uint16_t initial_sounds[256],expected_sounds[256];
    unsigned sounds=sound_count,serviced=serviced_sound_count,expected_sound_count=0;
    initial_front=front;initial_runtime=runtime;memcpy(initial_raw,raw,sizeof raw);
    memcpy(initial_pads,pads,sizeof pads);memcpy(initial_sounds,sound_log,sizeof sound_log);
    for(unsigned actual=0;actual<2u;actual++){
        front=initial_front;runtime=initial_runtime;memcpy(raw,initial_raw,sizeof raw);
        memset(pads,0,sizeof pads);sound_count=sounds;serviced_sound_count=serviced;
        memcpy(sound_log,initial_sounds,sizeof sound_log);
        input_observation_count=0;memset(input_observations,0,sizeof input_observations);
        front.config.input_word=changing_input_first;
        front.combat_coin_clock=(wf_jo_clock_t){fields,0,1,1};
        if(!actual)front.driver.platform.read_bus=reference_coin_input;
        unsigned ports=input_port_queries,keys=input_key_queries;
        assert(wf_jo_frontend_combat_coins(&front)==WF_OK);
        assert(input_observation_count==51u&&front.combat_coin_irqs==initial_front.combat_coin_irqs+17u);
        assert(input_port_queries-ports==4u&&input_key_queries-keys==32u);
        assert(front.config.input_word==changing_input_next);
        if(!actual){
            expected_front=front;expected_front.driver.platform.read_bus=initial_front.driver.platform.read_bus;
            expected_runtime=runtime;memcpy(expected_raw,raw,sizeof raw);
            memcpy(expected_observations,input_observations,sizeof input_observations);
            expected_sound_count=sound_count;memcpy(expected_sounds,sound_log,sizeof sound_log);
        }else{
            assert(!memcmp(&front,&expected_front,sizeof front));
            assert(!memcmp(&runtime,&expected_runtime,sizeof runtime));
            assert(!memcmp(raw,expected_raw,sizeof raw));
            assert(!memcmp(input_observations,expected_observations,sizeof input_observations));
            assert(sound_count==expected_sound_count&&!memcmp(sound_log,expected_sounds,sizeof sound_log));
        }
    }
    front=initial_front;runtime=initial_runtime;memcpy(raw,initial_raw,sizeof raw);
    memcpy(pads,initial_pads,sizeof pads);memcpy(sound_log,initial_sounds,sizeof sound_log);
    sound_count=sounds;serviced_sound_count=serviced;
    puts("PASS sampled input mutation: 51 separate callbacks, callback replacement, CPU/RAM mutations, one physical snapshot, exact IRQ result");
}
static void audio_store_order_test(void)
{
    wf_jo_frontend_t initial=front;unsigned sounds=sound_count,serviced=serviced_sound_count;
    uint16_t initial_sounds[256];memcpy(initial_sounds,sound_log,sizeof sound_log);
    for(unsigned fail=0;fail<3u;fail++){
        front=initial;front.control_ports[12]=0x12u;front.control_ports[13]=0x34u;
        sound_count=sounds;serviced_sound_count=serviced;store_audio_order=0;
        store_audio_fail=fail;store_audio_watch=1;
        wf_status_t status=front.driver.platform.store_bus(&front,0x114000cu,0x1234312au,2u);
        store_audio_watch=0;
        assert(status==(fail?WF_BLOCKED:WF_OK)&&store_audio_order==(fail==1u?1u:2u));
        unsigned expected=fail==1u?0x5678u:fail==2u?0x9abcu:0x312au;
        assert(((unsigned)front.control_ports[12]<<8|front.control_ports[13])==expected);
        if(fail)assert(front.blocked_dependency==0x14000cu&&front.blocked_pc==front.driver.pc);
        else assert(!front.blocked_dependency);
    }
    front=initial;sound_count=sounds;serviced_sound_count=serviced;
    memcpy(sound_log,initial_sounds,sizeof sound_log);
    puts("PASS audio bus store: address mask, value truncation, ordered services, delayed word write, both failure boundaries");
}
static void coin_recovery_test(void)
{
    /* The Royal terminal wait left 66 cabinet periods (1,122 IRQ2s).
     * Compare the production recovery with 66 ordinary one-period calls;
     * no IRQ callback, native state update or input request is substituted. */
    static wf_jo_frontend_t initial_front,expected_front,partial_front;
    static wf_runtime_t initial_runtime,expected_runtime,partial_runtime;
    static uint8_t initial_raw[sizeof raw],expected_raw[sizeof raw],partial_raw[sizeof raw];
    static uint32_t expected_inputs[sizeof coin_inputs/sizeof coin_inputs[0]];
    unsigned sounds=sound_count,serviced=serviced_sound_count,expected_count,partial_count=0;
    initial_front=front;initial_runtime=runtime;memcpy(initial_raw,raw,sizeof raw);
    memset(pads,0,sizeof pads);coin_audio_watch=1;coin_audio_fail_at=0;
    coin_audio_calls=coin_audio_max_gap=coin_input_count=0;
    coin_audio_last_irqs=front.combat_coin_irqs;
    front.combat_coin_clock=(wf_jo_clock_t){fields,0,0,66};
    assert(wf_jo_frontend_combat_coins(&front)==WF_OK&&coin_audio_calls==0);
    for(unsigned group=0;group<66u;++group){
        front.combat_coin_clock.pending=1;
        assert(wf_jo_frontend_combat_coins(&front)==WF_OK);
        assert(coin_audio_calls==0); /* No extra service on the usual path. */
        if(group==4u){partial_front=front;partial_runtime=runtime;
            memcpy(partial_raw,raw,sizeof raw);partial_count=coin_input_count;}
    }
    expected_front=front;expected_runtime=runtime;memcpy(expected_raw,raw,sizeof raw);
    expected_count=coin_input_count;memcpy(expected_inputs,coin_inputs,expected_count*sizeof coin_inputs[0]);
    assert(sound_count==sounds);
    for(unsigned fail=0;fail<2u;++fail){
        front=initial_front;runtime=initial_runtime;memcpy(raw,initial_raw,sizeof raw);
        memset(pads,0,sizeof pads);coin_audio_calls=coin_audio_max_gap=coin_input_count=0;
        coin_audio_last_irqs=front.combat_coin_irqs;coin_audio_fail_at=fail?5u:0u;
        front.combat_coin_clock=(wf_jo_clock_t){fields,0,66,66};
        unsigned ports=input_port_queries,keys=input_key_queries;
        wf_status_t status=wf_jo_frontend_combat_coins(&front);
        unsigned tail=front.combat_coin_irqs-coin_audio_last_irqs;
        if(tail>coin_audio_max_gap)coin_audio_max_gap=tail;
        assert(input_port_queries-ports==4u&&input_key_queries-keys==32u);
        if(coin_audio_max_gap>17u)
            fprintf(stderr,"Audio starved across %u IRQ2s; expected service between17-IRQ groups\n",coin_audio_max_gap);
        assert(coin_audio_max_gap==17u);
        assert(sound_count==sounds);
        if(!fail){
            assert(status==WF_OK&&coin_audio_calls==65u);
            assert(coin_input_count==expected_count);
            assert(!memcmp(&front,&expected_front,sizeof front));
            assert(!memcmp(&runtime,&expected_runtime,sizeof runtime));
            assert(!memcmp(raw,expected_raw,sizeof raw));
        }else{
            assert(status==WF_BLOCKED&&coin_audio_calls==5u);
            assert(front.blocked_dependency==0x2052u&&front.blocked_pc==front.driver.pc);
            assert(front.combat_coin_clock.pending==61u&&coin_input_count==partial_count);
            wf_jo_frontend_t comparable=front;
            comparable.blocked_dependency=partial_front.blocked_dependency;
            comparable.blocked_pc=partial_front.blocked_pc;comparable.combat_coin_clock.pending=0;
            assert(!memcmp(&comparable,&partial_front,sizeof comparable));
            assert(!memcmp(&runtime,&partial_runtime,sizeof runtime));
            assert(!memcmp(raw,partial_raw,sizeof raw));
        }
        assert(!memcmp(coin_inputs,expected_inputs,coin_input_count*sizeof coin_inputs[0]));
        /* The complete path must release the snapshot on success and error. */
        coin_audio_watch=0;pads[0]=JO_KEY_UP;ports=input_port_queries;
        uint32_t word;
        assert(front.driver.platform.read_bus(&front,0x140020u,&word,2)==WF_OK);
        assert(input_port_queries>ports&&(word&0xffu)==(0xffu&~4u));
        coin_audio_watch=1;
    }
    coin_audio_watch=0;coin_audio_fail_at=0;memset(pads,0,sizeof pads);
    front=initial_front;runtime=initial_runtime;memcpy(raw,initial_raw,sizeof raw);
    sound_count=sounds;serviced_sound_count=serviced;
    puts("PASS cabinet audio recovery: 66 periods, ordered1122 IRQ2s, one input snapshot, bounded service, preserved state and error cleanup");
}
static void cabinet_test(unsigned test_start)
{
    active_inputs=0;coin_recovery_test();input_mutation_test();audio_store_order_test();
    wf_runtime_t before=runtime;unsigned sounds=sound_count;
    assert(wf_jo_frontend_combat_coins(&front)==WF_OK);
    pads[0]=JO_KEY_C;coin_period();pads[0]=0;coin_period();
    assert(runtime.g.credit_count_004f==before.g.credit_count_004f+1u);
    assert(sound_count==sounds+1u&&sound_log[sounds]==0x312au);
    before.g.credit_count_004f=runtime.g.credit_count_004f;
    before.g.credit_remainder_004e=runtime.g.credit_remainder_004e;
    assert(!memcmp(&before,&runtime,sizeof runtime));
    /* Holding C does not auto-repeat credits. All17 raster service invocations
     * still occur for each rational cabinet period, independently of combat. */
    pads[0]=JO_KEY_C;coin_period();coin_period();coin_period();
    assert(sound_count==sounds+1u);pads[0]=0;coin_period();
    assert(sound_count==sounds+2u);
    if(!test_start){
        assert(wf_payment_consume_055a(&runtime,0u));
        uint8_t left=runtime.g.credit_count_004f;coin_period();
        assert(runtime.g.credit_count_004f==left); /* stale raw cannot refund it */
        pads[0]=JO_KEY_START;unsigned accepted=123;
        assert(wf_jo_frontend_combat_start(&front,&accepted)==WF_OK&&!accepted);
        assert(front.handed_off&&front.driver.phase==WF_R18_DRIVER_AC0);
    }else{
        /* Isolated $89E IRQ seam with a real attract F8C image. No victorious
         * actor/result bytes are injected; the original $978 spends the coin. */
        assert(runtime.g.credited_route==0u);
        uint32_t generation=front.driver.generation;uint8_t credits=runtime.g.credit_count_004f;
        pads[0]=JO_KEY_START;unsigned accepted=0;
        assert(wf_jo_frontend_combat_start(&front,&accepted)==WF_OK&&accepted);
        assert(!front.handed_off&&front.driver.phase==WF_R18_DRIVER_MODE_PLATE&&front.driver.pc==0x52beu);
        assert(front.driver.generation==generation+1u&&runtime.g.credit_count_004f==credits-1u);
        assert(raw[0x7c]&128u);assert(raw[0x5b0]==128u&&raw[0x5b1]==0u);
        assert(raw[0x63a]==0u&&raw[0x63b]==0x14u&&raw[0x63c]==0u&&raw[0x63d]==0x20u);
        for(unsigned slot=1;slot<9u;slot++)assert(raw[0x5b0u+slot*0x10cu]==0u);
    }
    memset(pads,0,sizeof pads);
    printf("PASS cabinet bridge: original coin pulses,17 IRQ2 per physical arcade period, credit-spend coherence, %s\n",
           test_start?"attract START978 -> 8AE -> 52BE":"paid-match START stays in gameplay");
}
int main(int argc,char **argv)
{
    assert(argc>=3&&argc<=5);wanted_royal=(unsigned)atoi(argv[1]);field_stride=(unsigned)atoi(argv[2]);
    unsigned post=argc>=4?(unsigned)strtoul(argv[3],NULL,0):0;
    unsigned initial_high=argc==5?(unsigned)strtoul(argv[4],NULL,0):0;
    assert(initial_high<=1u);
    assert(!post||post==0x1b4eu||post==0x1afcu||post==0x434u||post==0x978u);
    assert(wanted_royal<2u&&field_stride>=1u&&field_stride<=4u);
    wf_host_assets_init();wf_r18_present_init(wf_jo_assets_file_read,NULL,0,NULL,0);
    size_t original_cache_bytes=wf_jo_assets_sprite_cache_active_bytes();
    assert(wf_r18_hud_bind(&video,wf_jo_assets_file_read)==WF_OK);
    size_t count=wf_gameplay_bindings_build(&gameplay,bindings,48);assert(count);
    wf_runtime_init(&runtime,bindings,count);
    wf_jo_frontend_config_t config={.runtime=&runtime,.video=&video,.world=&world,.display=&display,
        .clock=&clock_state,.ram=raw,.ram_bytes=sizeof raw,.scene_blob=scene,.scene_capacity=sizeof scene,
        .priority_140010=&priority,.render_counts=gameplay.render_queue.count,.combat_read_ram=combat_ram,
        .input_word=fixture_input,.records_ready=records_ready};
    wf_jo_frontend_config_t overlap=config;overlap.ram=scene;
    assert(wf_jo_frontend_init(&front,&overlap,1085u)==WF_BAD_STATE);
    assert(wf_jo_frontend_init(&front,&config,1085u)==WF_OK);physical_inputs();
    require_dirty_frames=getenv("WF_JO_TEST_DIRTY_FRAMES")!=NULL;
    if(require_dirty_frames){native_commit=front.driver.platform.commit;front.driver.platform.commit=changed_frame;}
    /* An application callback may have already queued this tag. The first
     * synchronous wait must close it, retaining the newer staged scene. */
    queued=1;wf_host_assets_play();
    unsigned steps=0;
    while(front.driver.phase!=WF_R18_DRIVER_TITLE||front.driver.scene.title.pc!=0x837eu){
        int status=wf_r18_frontend_driver_step(&front.driver);
        if(status<0){fprintf(stderr,"boot blocked %x/%x adapter %x/%x\n",front.driver.pc,
            front.driver.blocked_dependency,front.blocked_pc,front.blocked_dependency);return 2;}
        assert(++steps<10000u);
    }
    assert(front.arcade_ticks==1172u);
    /* IRQ2 still runs17 times per admitted tick; physical controls should
     * no longer be rescanned hundreds of times for one batch of IRQs. */
    assert(input_port_queries<front.arcade_ticks*16u);
    wp1980_frontend_input_begin(front.arcade_ticks);active_inputs=post!=0x978u;
    wf_status_t status=wf_jo_frontend_run(&front);
    if(status!=WF_OK){fprintf(stderr,"frontend blocked %x/%x core %x/%x match %x/%x tick%u\n",
        front.blocked_pc,front.blocked_dependency,front.driver.pc,front.driver.blocked_dependency,
        front.match.blocked_pc,front.match.blocked_dependency,front.arcade_ticks);return 2;}
    if(post!=0x978u)assert(front.arcade_ticks==(wanted_royal?1771u:4510u));
    assert(wf_jo_assets_sprite_cache_active_bytes()==original_cache_bytes);
    if(post!=0x978u){
        fprintf(stderr,"SCENE_PREPARE begun=%u steps=%u bytes=%u completed=%u hits=%u cancelled=%u errors=%u\n",
            wf_jo_scene_prepare_stats.begun,wf_jo_scene_prepare_stats.steps,
            wf_jo_scene_prepare_stats.decoded_bytes,wf_jo_scene_prepare_stats.completed,
            wf_jo_scene_prepare_stats.hits,wf_jo_scene_prepare_stats.cancelled,wf_jo_scene_prepare_stats.errors);
        assert(wf_jo_scene_prepare_stats.completed>=3u&&wf_jo_scene_prepare_stats.hits>=3u);
        assert(!wf_jo_scene_prepare_stats.errors);
    }
    assert(runtime.frame_stage==3u&&runtime.g.frame_counter==0u);
    assert(records_notifications==0u);
    if(post!=0x978u)assert(!!(runtime.g.mode_0161&1u)==!!wanted_royal&&runtime.g.credited_route==0x8000u);
    else assert(runtime.g.credited_route==0u);
    assert(front.driver.ram==raw&&front.handed_off&&front.bound_world==0u);
    assert(video.read_ram==combat_ram&&video.read_user==NULL);
    assert(!world.animation_count[0]&&!world.animation_count[1]);
    assert(front.raster_irqs==17u*front.arcade_ticks&&front.vblank_irqs==front.arcade_ticks);
    assert(layers_state==7u&&opaque_state&&sprites_state&&gate_count&&sound_count);
    uint32_t minimum=(uint32_t)(((uint64_t)front.arcade_ticks*39168u+37608u)/37609u);
    assert(fields>=minimum);
    if(field_stride==1u)assert(fields<minimum+field_stride);
    assert((uint64_t)front.arcade_ticks+clock_state.pending==(uint64_t)fields*37609u/39168u);
    unsigned elapsed_fields=fields;
    assert(wf_jo_frontend_flush(&front)==WF_OK);unsigned before=commits;
    assert(wf_jo_frontend_flush(&front)==WF_OK&&commits==before);
    /* A second portable submit is staged, not appended to the frozen tag. */
    assert(front.driver.platform.submit_records(&front,NULL,0)==WF_OK);
    assert(wf_jo_frontend_flush(&front)==WF_BLOCKED&&front.pending);
    __jo_core_synch();assert(wf_jo_frontend_flush(&front)==WF_OK&&!front.pending);
    assert(!front.blocked_dependency);
    if(require_dirty_frames){assert(published_state==dirty_state);puts("PASS dirty frontend frames: each completed IRQ3 state published before the next IRQ3");}
    printf("PASS Jo frontend %s: ticks=%u fields=%u stride=%u IRQ2=%u IRQ3=%u commits=%u sounds=%u\n",
        wanted_royal?"Royal":"TAG",front.arcade_ticks,elapsed_fields,field_stride,
        front.raster_irqs,front.vblank_irqs,commits,sound_count);
    if(post==0x434u||post==0x978u){cabinet_test(post==0x978u);return 0;}
    if(post){
        /* Enter the actual adapter from its real F8C state. No victory/result
         * actor bytes are fabricated. Advance only the host field counter to
         * test that combat's elapsed time is not replayed as frontend IRQs. */
        active_inputs=0;fields+=257u;
        uint32_t started=fields,ticks=front.arcade_ticks,generation=front.driver.generation;
        uint32_t remainder=clock_state.remainder,irq2=front.raster_irqs,irq3=front.vblank_irqs;
        unsigned sounds=sound_count;
        status=wf_jo_frontend_after_combat_at_level(&front,post,initial_high);
        if(status!=WF_OK){fprintf(stderr,"post adapter blocked owner=%x pc=%x dependency=%x core=%x/%x\n",
            post,front.blocked_pc,front.blocked_dependency,front.driver.pc,front.driver.blocked_dependency);return 2;}
        uint32_t elapsed=front.arcade_ticks-ticks;
        printf("POST Jo owner=%x mode=%s initial_high=%u ticks=%u fields=%u generation_delta=%u IRQ2=%u IRQ3=%u\n",
               post,wanted_royal?"Royal":"TAG",initial_high,elapsed,fields-started,front.driver.generation-generation,
               front.raster_irqs-irq2,front.vblank_irqs-irq3);fflush(stdout);
        assert(front.driver.ram==raw&&!front.handed_off);
        assert(records_notifications==1u&&"Completed ranking must notify persistence once");
        assert(front.driver.phase==WF_R18_DRIVER_BOOT&&front.driver.pc==0x6fcu);
        assert(front.driver.generation==generation+1u);
        assert(!memcmp(front.driver.a,front.match.machine.a,sizeof front.driver.a));
        assert(front.raster_irqs-irq2==17u*elapsed&&front.vblank_irqs-irq3==elapsed);
        assert((uint64_t)elapsed+clock_state.pending==((uint64_t)(fields-started)*37609u+remainder)/39168u);
        if(post==0x1b4eu){
            assert(elapsed==2504u);
            assert(sound_count==sounds+2u&&sound_log[sounds]==0x3111u&&sound_log[sounds+1u]==0x311fu);
        }else{
            /* Original $2A06 consumes an extra edge only for LOW entry. */
            assert(elapsed==98u-initial_high&&sound_count==sounds);
        }
        assert(!front.blocked_dependency);
        puts("PASS Jo post adapter: raw alias, LEVEL/A[], generation, clock reanchor and native endpoint");
    }
    return 0;
}
