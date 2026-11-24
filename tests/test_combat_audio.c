/* The game emits through its production gameplay binding, WFSX adapter and
 * Jo SCSP descriptor writer. Only the CD/music and physical registers are
 * host boundaries; tests/test_combat_audio.py extracts the actual Jo code. */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "wf_gameplay_bindings.h"
#include "wf_r17_dispatch_1c03e.h"
#include "wf_collision_dispatch.h"
#include "wf_jo_audio.h"
#include "jo/audio.h"
#include "jo/sega_saturn.h"
#include "audio_header_stub.h"

extern void wf_test_driver_reset(void);
extern unsigned wf_test_driver_register(unsigned,unsigned);
extern unsigned wf_test_driver_pcm_be16(unsigned);
extern unsigned __jo_core_vblank_count;
static unsigned now,commands;
static uint16_t emitted[64];
static uint8_t *sfx;
static size_t sfx_bytes;
static wf_runtime_t runtime;
static wf_gameplay_binding_context_t gameplay;
static wf_status_t output_status;
static bool poison_registers;
unsigned jo_get_ticks(void){return now;}
void *jo_malloc(unsigned n){return malloc(n);}
void jo_free(void*p){free(p);}
bool jo_audio_reserve_music(void){return true;}
bool jo_audio_prepare_music(const char*n,jo_audio_prepared_music*p){(void)n;(void)p;return false;}
bool jo_audio_prepare_music_from_header(const char*n,const unsigned char*h,unsigned b,
                                      unsigned z,jo_audio_prepared_music*p)
{wf_test_check_music_header(n,h,b,z);return jo_audio_prepare_music(n,p);}
bool jo_audio_cache_music_prefix(const char*n,jo_audio_prepared_music*p,void*s,unsigned c)
{(void)n;(void)p;(void)s;(void)c;return false;}
bool jo_audio_prime_prepared_music(const jo_audio_prepared_music*p){(void)p;return false;}
bool jo_audio_is_music_ready(void){return false;}
bool jo_audio_is_music_pending(void){return false;}
bool jo_audio_is_music_playing(void){return false;}
bool jo_audio_play_prepared_music(const jo_audio_prepared_music*p,bool loop){(void)p;(void)loop;return false;}
void jo_audio_set_music_volume(unsigned char v){(void)v;}
void jo_audio_stop_music(void){}
void jo_audio_service_music(void){}
unsigned jo_audio_get_music_io_errors(void){return 0;}
unsigned jo_audio_get_music_start_ticks(void){return 0;}
unsigned jo_audio_get_music_underruns(void){return 0;}

static wf_status_t capture(wf_runtime_t*rt,uint16_t raw,void*u){
    assert(commands<64);emitted[commands++]=raw;
    if(poison_registers)memset(&rt->m68k_regs,0x5a,sizeof rt->m68k_regs);
    if(output_status!=WF_OK)return output_status;
    return wf_r18_scsp_audio(rt,raw,u);
}
static void reset(void){
    wf_test_driver_reset();now=commands=0;
    poison_registers=false;output_status=WF_OK;
    memset(&runtime,0,sizeof runtime);memset(&gameplay,0,sizeof gameplay);
    runtime.g.credited_route=1;runtime.m68k_regs.valid=1;
    gameplay.audio_command=capture;
    assert(wf_jo_audio_init(sfx,sfx_bytes));
    /* Install the production device at its normal phase boundary. */
    assert(wf_phase_collision_24062(&runtime,0x24062,&gameplay)==WF_OK);
}
static void expect_slot(unsigned voice){
    unsigned slot=voice*2u,control=wf_test_driver_register(slot,0);
    unsigned address=(control&15u)*65536u+wf_test_driver_register(slot,2);
    assert(control&JO_SCSP_KEY_ON);
    assert(!(control&(JO_SCSP_PCM_8BITS|JO_SCSP_LOOP_NORMAL)));
    assert(wf_test_driver_register(slot,6)>100u);
    assert(wf_test_driver_register(slot,8)==JO_SCSP_ATTACK_RATE_MAX);
    assert((wf_test_driver_register(slot,0x16)&0xe000u)==0xe000u);
    assert(wf_test_driver_register(slot,0x10)==0x69ceu); /* 8000 Hz, OCT=-3. */
    unsigned nonzero=0;for(unsigned i=0;i<512u;i+=2)nonzero|=wf_test_driver_pcm_be16(address+i);
    assert(nonzero);
}
static void collision(void){
    wf_actor_raw_t*a=&runtime.actors[0],*b=&runtime.actors[1];
    wf_set_be16(a,WF_A_ACTIVE,0x8000);wf_set_be16(b,WF_A_ACTIVE,0x8000);
    wf_set_be16(a,WF_A_STATE,0x8005);wf_set_be16(b,WF_A_STATE,0x8000);
    wf_set_be16(a,WF_A_PROFILE,7);wf_set_be16(b,WF_A_PROFILE,1);
    wf_set_be16(a,WF_A_X,0x280);wf_set_be16(b,WF_A_X,0x270);
    wf_set_be16(a,WF_A_Y,0x160);wf_set_be16(b,WF_A_Y,0x160);
    wf_set_be16(a,WF_A_Z,0x140);wf_set_be16(b,WF_A_Z,0x140);
    assert(wf_phase_collision_24062(&runtime,0x24062,&gameplay)==output_status);
    assert(gameplay.collision_24062.accepted_pairs==1);
}
static void trace(const char*kind,unsigned a,unsigned b,unsigned c,unsigned d){
    printf("TRACE %s %u %u %u %u",kind,a,b,c,d);
    for(unsigned i=0;i<commands;++i)printf(" %u",emitted[i]);
    putchar('\n');
}
static void landing(unsigned subtype,unsigned frame,unsigned ringside,unsigned contact){
    wf_actor_raw_t*a=&runtime.actors[0];
    wf_r17_dispatch_1c03e_ctx_t dispatch={0};
    wf_set_be16(a,WF_A_LATCHED,0x8004);wf_set_be16(a,WF_A_STATE,0x8004);
    wf_set_be16(a,WF_A_STATE4,(uint16_t)subtype);
    wf_set_be16(a,WF_A_ANIM_INDEX,(uint16_t)frame);wf_set_be16(a,WF_A_ANIM_COUNTER,1);
    wf_set_be16(a,WF_A_CONTACT,(uint16_t)contact);
    wf_set_u8(a,WF_A_FLAGS33,ringside?4:0);
    wf_set_be16(a,WF_A_X,0x280);wf_set_be16(a,WF_A_Y,0x160);wf_set_be16(a,WF_A_Z,0x140);
    assert(wf_r17_dispatch_1c03e_bounded(&runtime,a,&dispatch)==WF_OK);
    trace("landing",subtype,frame,ringside,contact);
}
static void dispatch_event(unsigned domain,unsigned subtype,unsigned latched,
        unsigned frame,unsigned timer,unsigned contact,unsigned flags,
        unsigned mode,unsigned scene,unsigned contact_latch){
    reset();runtime.g.mode_0161=(uint8_t)mode;runtime.g.global_007e=(uint16_t)scene;
    runtime.g.global_007f=(uint8_t)scene;
    wf_actor_raw_t*a=&runtime.actors[0],*b=&runtime.actors[1];
    wf_r17_dispatch_1c03e_ctx_t dispatch={0};
    wf_set_be16(a,WF_A_LATCHED,(uint16_t)(domain|(latched?0x8000u:0)));
    wf_set_be16(a,WF_A_STATE,(uint16_t)(0x8000u|domain));
    wf_set_be16(a,WF_A_STATE4,(uint16_t)subtype);
    wf_set_be16(a,WF_A_TIMER44,(uint16_t)(domain==8?subtype:contact_latch));
    wf_set_be16(a,WF_A_ANIM_INDEX,(uint16_t)frame);wf_set_be16(a,WF_A_ANIM_COUNTER,(uint16_t)timer);
    wf_set_be16(a,WF_A_CONTACT,(uint16_t)contact);wf_set_u8(a,WF_A_FLAGS33,(uint8_t)flags);
    wf_set_be16(a,WF_A_X,0x280);wf_set_be16(a,WF_A_Y,0x160);wf_set_be16(a,WF_A_Z,0x140);
    wf_set_be32(a,WF_A_LINK,wf_actor_rom_base(1));wf_set_be32(b,WF_A_LINK,wf_actor_rom_base(0));
    assert(wf_r17_dispatch_1c03e_bounded(&runtime,a,&dispatch)==WF_OK);
    printf("TRACE dispatch %u %u %u %u %u %u %u %u %u %u",domain,subtype,latched,frame,timer,contact,flags,mode,scene,contact_latch);
    for(unsigned i=0;i<commands;++i)printf(" %u",emitted[i]);
    putchar('\n');
}
int main(int argc,char**argv){
    assert(argc==2);FILE*f=fopen(argv[1],"rb");assert(f);
    assert(!fseek(f,0,SEEK_END));sfx_bytes=(size_t)ftell(f);rewind(f);
    sfx=malloc(sfx_bytes);assert(sfx&&fread(sfx,1,sfx_bytes,f)==sfx_bytes);fclose(f);
    reset();collision();
    assert(commands==2);assert(emitted[0]==0x312a&&emitted[1]==0x3132);
    assert(wf_r18_scsp_service());
    assert(wf_jo_audio_diagnostic.physical_starts==2);
    expect_slot(0);expect_slot(1);
    trace("collision",1,0,0,0);
    puts("PASS: native punch collision -> commands 2A/32 -> real WFSX PCM -> Jo SCSP key-on");
    static const unsigned falls[][2]={{2,0},{3,0},{4,0},{5,0},{6,0},{7,0},
        {15,3},{19,0},{21,0},{23,0},{25,0},{26,0},{27,0},{33,0},{34,1},{35,0},{37,2},{38,0}};
    for(unsigned i=0;i<sizeof falls/sizeof falls[0];++i){
        for(unsigned outside=0;outside<2;++outside){
            reset();landing(falls[i][0],falls[i][1],outside,0x10);
            if(commands!=1){fprintf(stderr,"subtype %u frame %u: %u commands\n",falls[i][0],falls[i][1],commands);abort();}
            assert(emitted[0]==(outside?0x312d:0x3129));
            assert(wf_r18_scsp_service());expect_slot(0);
        }
    }
    reset();landing(19,0,0,0);assert(commands==0);
    puts("PASS: native landing owners choose 29/2D only on real ground contact");
    /* Every collision selector follows its ROM accept/reject gate. */
    static const uint8_t expected[][3]={{0,0,0},{2,0x2a,0x32},{1,0x2b,0},
        {0,0,0},{2,0x2b,0x32},{2,0x2b,0x32},{2,0x2e,0x32},{2,0x2b,0x32},
        {1,0x2b,0},{2,0x2b,0x32},{2,0x29,0x32},{2,0x2b,0x32},
        {2,0x2b,0x32},{2,0x2b,0x32},{2,0x29,0x32}};
    for(unsigned selector=0;selector<15u;++selector){
        reset();wf_actor_raw_t*a=&runtime.actors[0],*b=&runtime.actors[1];
        wf_set_be16(a,WF_A_PROFILE,7);wf_set_be16(b,WF_A_PROFILE,1);
        wf_set_be16(b,WF_A_FACING,0x8000);wf_set_u8(b,WF_A_FLAGS33,16);
        wf_collision_pair_ctx_t pair={a,b,NULL,1,7,1,&runtime};
        assert(wf_collision_first_apply((uint8_t)selector,&pair)==WF_COLL_ACCEPT);
        assert(commands==expected[selector][0]);
        for(unsigned i=0;i<commands;++i)assert(emitted[i]==(0x3100u|expected[selector][i+1u]));
        trace("selector",selector,0,0,0);
        if(selector&&selector!=6&&selector!=8){
            reset();a=&runtime.actors[0];b=&runtime.actors[1];
            wf_set_be16(a,WF_A_PROFILE,7);wf_set_be16(b,WF_A_PROFILE,0xc001);
            wf_set_be16(b,WF_A_FACING,0x8000);wf_set_u8(b,WF_A_FLAGS33,16);
            pair=(wf_collision_pair_ctx_t){a,b,NULL,1,7,1,&runtime};
            assert(wf_collision_first_apply((uint8_t)selector,&pair)==WF_COLL_REJECT);
            assert(!commands);trace("selector",selector,0xc0,0,0);
        }
    }
    puts("PASS: all 15 collision selectors preserve ROM command order and rejection gates");
    /* Restore audio only at the original side-contact, landing and step
     * branches. The optional ROM oracle checks every trace below. */
    for(unsigned owner=0;owner<2;++owner){
        unsigned subtype=owner?22:14,frame=owner?1:3;
        for(unsigned prior=0;prior<2;++prior)
            dispatch_event(4,subtype,1,frame,1,0x600,0,0,0,prior);
        for(unsigned mode=0;mode<=2;mode+=2){
            dispatch_event(4,subtype,1,frame,1,0x300,0,mode,0,0);
            for(unsigned outside=0;outside<=4;outside+=4)
                dispatch_event(4,subtype,1,frame,1,0x10,outside,mode,0,0);
        }
        dispatch_event(4,subtype,1,frame,1,0,0,0,0,0);
    }
    dispatch_event(4,19,1,0,1,0x300,0,0,0,0);
    dispatch_event(4,30,0,0,1,0,0,0,0,0);
    dispatch_event(4,23,0,0,1,0,0,0,0,0);
    dispatch_event(4,23,0,0,1,0,0,0,1,0);
    for(unsigned route=0;route<=8;route+=4)dispatch_event(8,route,0,0,1,0,0,0,0,0);
    dispatch_event(2,0,1,1,0,0,0,0,0,0);dispatch_event(2,0,1,4,0,0,0,0,0,0);
    dispatch_event(2,0,1,1,1,0,0,0,0,0);dispatch_event(2,0,1,0,0,0,0,0,0,0);
    dispatch_event(2,0,1,1,0,0x302,0,0,0,0);
    reset();runtime.g.credited_route=0;runtime.g.config_0066=0x20;collision();
    assert(!commands);trace("collision",0,0x20,0,0);
    reset();runtime.g.config_0066=0x20;collision();assert(commands==2);trace("collision",1,0x20,0,0);
    reset();poison_registers=true;
    wf_r17_m68k_shadow_t saved=runtime.m68k_regs;collision();
    assert(!memcmp(saved.d,runtime.m68k_regs.d,sizeof saved.d));
    assert(saved.valid==runtime.m68k_regs.valid);
    reset();output_status=WF_BAD_STATE;collision();assert(commands==1);
    assert(gameplay.last_blocked_dependency==0x2052u);
    puts("PASS: attract sound DIP, live register preservation and transport-error propagation");
    free(sfx);return 0;
}
